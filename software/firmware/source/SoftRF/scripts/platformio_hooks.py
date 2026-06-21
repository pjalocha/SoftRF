from pathlib import Path
import subprocess

Import("env")

project_dir = Path(env.subst("$PROJECT_DIR"))
libraries_dir = project_dir.parent / "libraries"
pioenv = env.subst("$PIOENV").lower()

if pioenv in ("techo", "t-echo"):
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


if pioenv in ("techo", "t-echo"):
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", build_uf2)
