# SoftRF

DIY, multifunctional, compatible, sub-1 GHz ISM band radio based proximity awareness system for general aviation.
<br>
[User Guide](https://github.com/moshe-braner/SoftRF/blob/master/software/firmware/documentation/SoftRF_MB_user_guide.txt)
<br>

<p><img src="https://github.com/moshe-braner/SoftRF/blob/master/software/firmware/documentation/T-Beam_MB149_.jpg"></p>

### About this fork

This repository is a fork of Moshe Braner's SoftRF project. The earlier
Moshe Braner version history is preserved below as the `vMB` series.
Changes made in this fork start a new `vPJ` version series.

The goal was to create two new variants of the SoftRF:
1. variant for ULM, gyro, gliders which interoperates best with FLARM, PilotAware and OGN-Tracker (all at the same time, no need to switch protocols)
2. variant for paragliders: FANET is the primary transmit and receive, but transmits other protocols to be visible by FLARM, ADS-L, PilotAware and OGN-Tracker

The current version is being tested with T-Echo, it compiles with T-Beam-Supreme and T-Motion but has not been tested there yet.

### PJ RF operating modes

This fork adds two main RF operating schemes. Both use ADS-L HDR in the
uplink slot, 200-450 ms after PPS.

**FLARM + ADS-L/LDR/HDR compatibility mode**

This is the default PJ mode for aircraft where FLARM/ADS-L visibility is the
main goal. It uses the FLARM/Latest protocol together with ADS-L, follows
Altitude Based Hopping (ABH), transmits FLARM in FLARM slots, ADS-L in ADS-L
slots, and LDR on the O-band channel. Reception uses the combined short-sync
receivers where possible, so FLARM slots can receive FLARM and ADS-L, and OGN
slots can receive OGN and ADS-L.

Settings:

```
protocol,7
altprotocol,8
flr_adsl,1
```

**FANET-primary compatibility mode**

This mode is intended for paragliders and other FANET-centric use. FANET is the
main protocol. The radio receives FANET in the normal slots outside the HDR
uplink slot, transmits FANET in slot 1, and uses slot 0 for compatibility
transmissions selected by ABH: FLARM, ADS-L, OGN, or LDR as appropriate. This
keeps the device visible to FLARM, ADS-L/LDR/HDR, PilotAware/OGN-Tracker style
systems while still treating FANET as the primary network.

Settings:

```
protocol,5
altprotocol,8
flr_adsl,1
```

Protocol values used above are: `5` = FANET, `7` = FLARM/Latest, `8` = ADS-L.
The `flr_adsl` setting enables the PJ compatibility scheduler and the combined
short-sync reception modes.

These settings can be edited in `settings.txt`, or changed over the serial
configuration interface with `$PSRFS`. For `$PSRFS`, use the long setting
labels shown above, for example:

```
$PSRFS,0,protocol,5*
$PSRFS,0,altprotocol,8*
$PSRFS,1,flr_adsl,1*
```

Using version field `1` in the last `$PSRFS` command saves the changed settings
and reboots. Do not use the internal two-letter shorthand labels in `$PSRFS`;
for example, use `altprotocol`, not `apaltprotocol`.

### Latest PJ additions:

* vPJ001: added PlatformIO build profiles for T-Echo, T-Beam Supreme, and T-Motion
* vPJ001: added ADS-L HDR uplink operation in the 200-450 ms PPS slot
* vPJ001: added ABH-based FLARM/ADS-L/LDR compatibility scheduling
* vPJ001: added FANET-primary mode with FANET reception outside the uplink slot and compatibility TX in slot 0
* vPJ001: added RF packet counters/rates and GPS satellite SNR reporting

### Previous MB additions:

* vMB203: now supports messaging (PFLAM)
* vMB202: using RadioLib, now supports the Sensecap T1000-E and Thinknode M3 (& M1)
* vMB179: added FANET id_method, auto-region by default, buzz at first GNSS fix
* vMB174: dual-protocol FANET (or P3I) plus FLARM (or ADS-L) modes
* vMB172: dual-protocol FLARM/ADS-L reception, some relaying in ADS-L protocol
* vMB171: revised relaying of ADS-B traffic, including "relay only" mode 
* vMB166: added the ADS-L protocol
* vMB160: setting to control the time traffic not-heard-from is reported
* vMB159: capability to periodically transmit in an alternate protocol
* vMB155: collect statistics on reception range by relative direction 
* vMB153: record compressed flight logs in flash memory (T-Beam & T-Echo)
* vMB152: settings stored in an editable text file
* vMB138: supports using add-on GNSS modules, on the T-Beam
* vMB130: supports the GNS5892R ADS-B receiver module, on the T-Beam
* vMB120: supports the latest 2024 radio protocol
* vMB110: added second serial port and data bridging (only on T-Beam)

### Beyond Lyusupov's version:

* Collision prediction for circling aircraft
* Can set aircraft ID for self, ID to ignore, and ID to follow
* Support 3-level collision alarms via buzzer
* Can configure two simultaneous NMEA output destinations
* Settable baud rate for serial output
* Estimates wind while circling, uses for collision prediction
* Corrected frequency hopping and time slots
* Airborne devices can relay radio packets from landed aircraft
* Can adjust SoftRF settings within T-Echo (without an app)

### And on the T-Beam specifically: 

* Louder buzzer via 2-pin differential drive, or external
* Collision-danger traffic VOICE warnings!
* Includes strobe-control logic
* Option to connect to ambient WiFi network instead of creating one
* Option to send data as TCP client instead of TCP server
* Specify server's IP address for TCP client, and choice of 2 ports
* Ability to use WiFi TCP & UDP NMEA outputs simultaneously
* These WiFi options allow wireless output to XCvario
* Detect and use OLED display on either I2C port
* Winch mode ("aircraft type") with variable reported altitude

### What is here:

Source code, and compiled binaries for [ESP32 (T-Beam)](https://github.com/moshe-braner/SoftRF/tree/master/software/firmware/binaries/ESP32/SoftRF) and [nRF52 (T-Echo, M1, M3 & T1000-E)](https://github.com/moshe-braner/SoftRF/tree/master/software/firmware/binaries/nRF52840/SoftRF/MassStorage) (only).  Note: a new T1000-E requires [downgrading the bootloader](https://raw.githubusercontent.com/moshe-braner/SoftRF/refs/heads/master/software/firmware/documentation/SoftRF_MB_user_guide.txt) before loading SoftRF.
<br>
<br>

[Documentation files](https://github.com/moshe-braner/SoftRF/tree/master/software/firmware/documentation).
<br>
<br>

Modified version of [SkyView](https://github.com/moshe-braner/SoftRF/tree/master/software/firmware/binaries/ESP32/SkyView)
<br>
[SkyStrobe](https://github.com/moshe-braner/SoftRF/tree/master/software/firmware/binaries/ESP32/SkyStrobe) - a controller for a visibility strobe (and more)
<br>
<br>

For discussions join the [SoftRF Community](https://groups.google.com/g/softrf_community).
<br>
<br>

For additional info see also [Lyusupov's repository](https://github.com/lyusupov/SoftRF).
