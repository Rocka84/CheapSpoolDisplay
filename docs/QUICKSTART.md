# Quickstart Guide

Get your CheapSpoolDisplay up and running from scratch.

## 1. Hardware Preparation
*   **Device**: ESP32-2432S028R (Cheap Yellow Display).
*   **NFC Reader**: Connect an **RC522** SPI module.
*   **Wiring**: 
    *   Sacrifice the SD card slot pins for the NFC reader.
    *   Connect: `SDA` (CS) -> `IO5`, `SCK` -> `IO18`, `MOSI` -> `IO23`, `MISO` -> `IO19`, `RST` -> `IO22`.
    *   *See [HARDWARE_SETUP.md](HARDWARE_SETUP.md) for details.*

## 2. Flash Firmware (Recommended)
You can flash the firmware directly from your browser (Chrome or Edge):
1.  Connect the CYD to your PC via USB.
2.  Open the **[CheapSpoolDisplay Web Installer](https://Rocka84.github.io/CheapSpoolDisplay)**.
3.  Click **Connect** and select your ESP32's COM port.
4.  Follow the prompts to **Install** the latest firmware.

## 3. Configuration
Once flashed, you can configure Wi-Fi and integrations directly in the Web Installer or via the Serial Terminal:
1.  **Web Installer**: Use the "Configure Wi-Fi" button after flashing.
2.  **Serial Terminal**: If you prefer, open a terminal (115200 baud) and use:
    *   `set wifi <SSID> <PASSWORD>`
    *   `set spoolman <URL>` (e.g., `http://192.168.1.50:8000`)
    *   `set u1_host <IP_OR_HOSTNAME>:7125` (Enable Snapmaker U1 loading)
    *   `set tools <1-16>` (Set number of toolheads)
    *   `set timeout <seconds>` (Screen auto-off timer)
    *   `get config` (To verify all settings)

---
*For more detailed configuration options, see [CONFIGURATION.md](CONFIGURATION.md).*
