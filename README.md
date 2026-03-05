# CheapSpoolDisplay

CheapSpoolDisplay is a firmware project for the ESP32 Cheap Yellow Display (CYD) that allows you to scan, view, and organize your 3D printer filament spools using the OpenSpool NFC tag format.

## Features
- **NFC Tag Scanning**: Reads NTAG215/216 NFC tags formatted via the OpenSpool JSON specification using a connected MFRC522 SPI module.
- **Visual Interface**: Provides a modern, touch-friendly UI powered by LVGL to display the filament Brand, Type, Spool ID, and material color.
- **Snapmaker Webhook Integration**: Configure a target URL to send POST payloads directly from the device after picking a specific 3D printer toolhead (Tool 0-3).
- **Persistent Configuration**: Serial Terminal allows you to program Wi-Fi credentials and Webhooks directly into flash memory over USB (`set ssid`, `set pass`, `set url`).
- **Web Installer**: A browser-based GUI built on ESP Web Tools to let users flash the firmware with zero CLI tools and use a browser-based serial console.
- **Power Management**: Intelligent power saving modes:
  - When powered via USB, the display automatically turns off after 60 seconds of inactivity and wakes on touch.
  - When powered by a battery, the device enters deep sleep to save power, waking via the physical CYD Reset button.

## Hardware Requirements
- **ESP32 Cheap Yellow Display (CYD)**
- **MFRC522 RFID SPI Module**
- (Optional) **Lithium Battery** 

## Documentation
Please see the `docs/` folder for detailed guides:
- [Hardware Setup & modifications](docs/HARDWARE_SETUP.md)
- [Testing documentation](docs/TESTING.md)
- [Web Installer Development](docs/WEB_INSTALLER_DEV.md)

## Installation & Flashing

### Option 1: ESP Web Tools (Recommended)
You can flash the firmware and set up your Wi-Fi directly from your PC browser (Chrome/Edge)! 

1. Navigate to the online Web Installer: **[Launch CheapSpoolDisplay Web Installer](https://Rocka84.github.io/CheapSpoolDisplay)**
2. Connect your CYD to your PC via USB.
3. Click **Connect & Flash Device**.
4. Use the integrated Serial Terminal on the webpage to set your Network and Webhook:
   - `set ssid YourWiFi`
   - `set pass YourWifiPassword`
   - `set url http://your-hook-url/`

### Option 2: PlatformIO
1. Clone this repository.
2. Open the project in *PlatformIO* (e.g. via VSCode).
3. Connect your CYD to your PC via USB.
4. Run the upload command: 
   ```bash
   pio run -t upload
   ```

## Testing
We utilize automated unit tests through PlatformIO (`Unity`). For detailed info, check [TESTING.md](docs/TESTING.md).

To run the **Desktop unit tests** locally on your PC:
```bash
pio test -e desktop
```

To run the **Embedded integration tests** directly on the connected CYD:
```bash
pio test -e esp32dev
```
