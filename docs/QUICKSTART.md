# Quickstart Guide

Get your CheapSpoolDisplay up and running from scratch.

## 1. Prepare the Hardware
See [HARDWARE_SETUP.md](HARDWARE_SETUP.md) for details.

*   **Device**: ESP32-2432S028R (Cheap Yellow Display).
*   **NFC Reader**: Connect an **RC522** SPI module.
*   **Wiring**: 
    *   Sacrifice the SD card slot pins for the NFC reader.
    *   Connect: `SDA` (CS) -> `IO5`, `SCK` -> `IO18`, `MOSI` -> `IO23`, `MISO` -> `IO19`, `RST` -> `IO22`.

## 2. Print the Case
- [Case Main](../cad/CheapSpoolDisplay%20-%20Case%20main.stl)
- [Case Lid](../cad/CheapSpoolDisplay%20-%20Case%20lid.stl)

## 3. Flash the Firmware
1.  Connect the CYD to your PC via USB.
2.  Open the **[CheapSpoolDisplay Web Installer](https://Rocka84.github.io/CheapSpoolDisplay)**.
3.  Click **Connect** and select your CYD's COM port.
4.  Follow the prompts to **Install** the latest firmware.

## 4. Configure the Device
See [CONFIGURATION.md](CONFIGURATION.md) for details.

1. Connect to the serial terminal using
    * the **Web Installer** (click **Logs & Console**)
    * any **Serial Terminal** (115200 baud)
2. Configuration Commands:
    *   `set wifi <SSID> <PASSWORD>` to configure your wireless network
    *   `set spoolman <URL>` to enable Spoolman enrichment (e.g., `http://192.168.1.50:8000`)
    *   `set u1_host <IP_OR_HOSTNAME>:7125` to enable Snapmaker U1 loading
    *   `set tools <1-16>` to set the number of toolheads
    *   `set timeout <seconds>` to adjust screen auto-off timings
    *   `get config` to verify your updated settings anytime

