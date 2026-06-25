from pathlib import Path
import subprocess
import struct
import zlib

Import("env")

project_dir = Path(env.subst("$PROJECT_DIR"))
libraries_dir = project_dir.parent / "libraries"
pioenv = env.subst("$PIOENV").lower()

if pioenv in ("tmotion", "t-motion"):
    env.BoardConfig().update("build.hwids", [["0x0483", "0x5740"]])

if pioenv in ("nrf52", "techo", "t-echo"):
    env.BuildSources(
        "$BUILD_DIR/lib_manual/ESP32_I2C_Bus",
        str(libraries_dir / "ESP32_I2C_Bus"),
        src_filter=["+<i2c_bus.cpp>"],
    )
    env.BuildSources(
        "$BUILD_DIR/lib_manual/PCF8563_Library",
        str(libraries_dir / "PCF8563_Library/src"),
        src_filter=["+<pcf8563.cpp>"],
    )
    env.BuildSources(
        "$BUILD_DIR/lib_manual/Seeed_Arduino_AHT20",
        str(libraries_dir / "Seeed_Arduino_AHT20/src"),
        src_filter=["+<ATH20.cpp>"],
    )
elif pioenv in ("tbeam-supreme", "t-beam-supreme"):
    env.BuildSources(
        "$BUILD_DIR/lib_manual/XPowersLib",
        str(libraries_dir / "XPowersLib/src"),
        src_filter=["+<XPowersLibInterface.cpp>"],
    )


def build_uf2(source, target, env):
    build_dir = Path(env.subst("$BUILD_DIR"))
    bin_path = build_dir / "firmware.bin"
    uf2_path = build_dir / "firmware.uf2"

    if not bin_path.exists():
        print(f"UF2: missing {bin_path}")
        return

    packages_dir = Path(env.subst("$PROJECT_PACKAGES_DIR"))
    tools = list(packages_dir.rglob("uf2conv.py"))
    if not tools:
        print("UF2: uf2conv.py was not found in the PlatformIO packages")
        return

    family = "0xADA52840"
    cmd = [
        env.subst("$PYTHONEXE"),
        str(tools[0]),
        str(bin_path),
        "-c",
        "-b",
        "0x26000",
        "-f",
        family,
        "-o",
        str(uf2_path),
    ]
    subprocess.check_call(cmd)
    print(f"UF2: wrote {uf2_path}")


def build_dfuse(source, target, env):
    build_dir = Path(env.subst("$BUILD_DIR"))
    bin_path = build_dir / "firmware.bin"
    dfu_path = build_dir / "firmware.dfu"
    flash_base = 0x08000000

    if not bin_path.exists():
        print(f"DFU: missing {bin_path}")
        return

    firmware = bin_path.read_bytes()
    if len(firmware) < 8:
        raise ValueError(f"DFU: {bin_path} is too short")

    stack, reset = struct.unpack_from("<II", firmware)
    if not (0x20000000 <= stack <= 0x20005000):
        raise ValueError(f"DFU: invalid initial stack pointer 0x{stack:08X}")
    if not (flash_base <= (reset & ~1) < flash_base + 0x30000) or not (reset & 1):
        raise ValueError(f"DFU: invalid reset vector 0x{reset:08X}")

    target_name = b"ST..."
    target_prefix = (
        b"Target"
        + struct.pack("<BI", 0, 1)
        + target_name.ljust(255, b"\0")
        + struct.pack("<II", len(firmware) + 8, 1)
    )
    element = struct.pack("<II", flash_base, len(firmware)) + firmware

    suffix_without_crc = struct.pack("<HHHH3sB", 0x0000, 0xDF11, 0x0483, 0x011A, b"UFD", 16)
    image_size = 11 + len(target_prefix) + len(element) + len(suffix_without_crc) + 4
    prefix = b"DfuSe" + struct.pack("<BIB", 1, image_size, 1)

    payload = prefix + target_prefix + element + suffix_without_crc
    crc = zlib.crc32(payload) ^ 0xFFFFFFFF
    dfu_path.write_bytes(payload + struct.pack("<I", crc))
    print(f"DFU: wrote {dfu_path} ({len(firmware)} bytes at 0x{flash_base:08X})")


if pioenv in ("nrf52", "techo", "t-echo"):
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", build_uf2)
elif pioenv in ("tmotion", "t-motion"):
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", build_dfuse)
