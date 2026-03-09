# CheapSpoolDisplay

CheapSpoolDisplay is a firmware project for the ESP32 Cheap Yellow Display (CYD) that allows you to scan, view, and organize your 3D printer filament spools using the OpenSpool NFC tag format.

## Features
- **NFC Tag Scanning**: Reads NTAG215/216 NFC tags formatted via the OpenSpool JSON specification using a connected MFRC522 SPI module.
- **Visual Interface**: Provides a modern, touch-friendly UI powered by LVGL to display the filament Brand, Type, Spool ID, and material color.
- **Webhook Integration**: Configure a target URL to send POST payloads directly from the device to load the spool to the printer.
  - The "Load" button only appears if a Webhook URL is configured.
  - If multiple tools are configured (via `set tools <1-6>`), a tool selection dialog will appear after picking load.
- **Spoolman Data Enrichment**: Optionally connect to a [Spoolman](https://github.com/Donkie/Spoolman) server to fetch real-time filament names and remaining weight (rounded to 0.1g).
- **Persistent Configuration**: Serial Terminal allows you to program Wi-Fi credentials, Webhooks, Spoolman URLs, and tool counts directly into flash memory over USB (`set wifi`, `set webhook`, `set spoolman`, `set tools`).
- **Conditional Connectivity**: Wi-Fi is only active if a Webhook is configured, saving power for offline usage.
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
- [Spoolman Integration Setup](docs/SPOOLMAN_SETUP.md)
- [Testing documentation](docs/TESTING.md)
- [Web Installer Development](docs/WEB_INSTALLER_DEV.md)

## Installation & Flashing

### Option 1: ESP Web Tools (Recommended)
You can flash the firmware and set up your Wi-Fi directly from your PC browser (Chrome/Edge)! 

1. Navigate to the online Web Installer: **[Launch CheapSpoolDisplay Web Installer](https://Rocka84.github.io/CheapSpoolDisplay)**
2. Connect your CYD to your PC via USB.
3. Click **Connect & Flash Device**.
4. Once flashed, use the integrated **Logs & Console** on the webpage to configure the device (see [Post-Flash Configuration](#post-flash-configuration)).

### Option 2: PlatformIO
1. Clone this repository and open it in VSCode with PlatformIO installed.
2. Connect your CYD to your PC via USB.
3. Run the upload command: 
   ```bash
   pio run -t upload
   ```
4. Once the upload finishes, open the **Serial Monitor** (or run `pio device monitor`) to configure the device (see [Post-Flash Configuration](#post-flash-configuration)).

## Post-Flash Configuration
Regardless of how you flash, the device stores your settings in non-volatile memory (NVS). Type `help` in the Serial Terminal to see all available commands.

### 1. Set Credentials
Use the following commands to set your network and webhook details:
- `set ssid YourWiFiName`
- `set pass YourWiFiPassword`
- `set webhook http://your-hook-url/webhook`
- `set spoolman http://your-spoolman-ip:8000`
- `set tools 4` (Set number of tools from 1 to 6)
- `get config` (To verify)

> [!NOTE]
> Wi-Fi will only initialize if a **Webhook** or **Spoolman** URL is set. If these fields are empty, the device remains offline.

### 2. Auto-Method Detection
The device automatically determines the HTTP method based on your Webhook URL:
- **GET Mode**: Triggered if the URL contains the `{spool_id}` placeholder (e.g. `http://api.com/load?spool={spool_id}`).
- **POST Mode**: Default mode. Sends a JSON payload: `{"spool_id": "...", "toolhead": X}`.

## Testing
We utilize automated unit tests through PlatformIO (`Unity`). For detailed info, check [TESTING.md](docs/TESTING.md).

To run the **Desktop unit tests** locally on your PC:
```bash
pio test -e desktop
```

To run the **Embedded integration tests** directly on the connected CYD:
```bash
pio test -e test_embedded
```

### UI Simulator (Desktop)
You can preview and test the "Premium" LVGL interface directly on your computer without hardware. This is perfect for UI development and layout testing.
**Requirement**: [SDL2](https://www.libsdl.org/) installed on your system.
```bash
# Build and run the desktop simulator
pio run -e simulator
./.pio/build/simulator/program
```
