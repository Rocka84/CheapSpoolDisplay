# CheapSpoolDisplay

CheapSpoolDisplay is a firmware project for the ESP32 Cheap Yellow Display (CYD) that allows you to scan, view, and organize your 3D printer filament spools using the OpenSpool NFC tag format.

## Features
- **NFC Tag Scanning**: Reads NTAG215/216 NFC tags formatted automatically via the OpenSpool JSON specification using a connected MFRC522 SPI module.
- **Visual Interface**: Provides a modern, touch-friendly UI powered by LVGL to display the filament Brand, Type, Spool ID, and material color.
- **Power Management**: Intelligent power saving modes:
  - When powered via USB, the display automatically turns off after 60 seconds of inactivity and wakes on touch.
  - When powered by a battery, the device enters deep sleep to save power, waking via the physical CYD Reset button.

## Hardware Requirements
- **ESP32 Cheap Yellow Display (CYD)**
- **MFRC522 RFID SPI Module**
- (Optional) **Lithium Battery** 

## Documentation
Please see the `docs/` folder for detailed board modification instructions and pinouts:
- [Hardware Setup & Power Modifications](docs/HARDWARE_SETUP.md)

## Installation
1. Clone this repository.
2. Open the project in *PlatformIO* (e.g. via VSCode).
3. Connect your CYD to your PC via USB.
4. Run the PlatformIO upload command: 
   ```bash
   pio run -t upload
   ```

## Testing
We utilize native and embedded testing frameworks through PlatformIO. 
To run the native unit tests for the OpenSpool Parser (which executes locally without flashing the hardware):
```bash
pio test -e native
```
