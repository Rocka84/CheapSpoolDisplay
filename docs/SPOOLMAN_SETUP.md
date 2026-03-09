# Spoolman Integration Setup

CheapSpoolDisplay can optionally fetch enriched data (filament name and weight) for any spool that has a `spool_id` on its NFC tag.

## Configuration

To enable Spoolman enrichment, you must configure your Spoolman server URL via the Serial Terminal.

1. Connect your device to your computer via USB.
2. Open a serial terminal (115200 baud).
3. Set your Spoolman URL:
   ```bash
   set spoolman http://192.168.1.100:8000
   ```
   *Replace with your actual Spoolman IP and port.*

4. Verify the configuration:
   ```bash
   get config
   ```

## Requirements

- **Wi-Fi**: The device must be configured with a valid SSID and Password (`set wifi ...`).
- **NFC Tag**: The tag must include a `spool_id` (OpenSpool format).
- **Spoolman API**: The device will attempt to reach `${SPOOLMAN_URL}/api/v1/spool/${SPOOL_ID}`.

## Display Features

When a tag is scanned and Spoolman data is found:
- **Filament**: Displays the friendly name of the filament.
  - **Note**: German umlauts and special characters in filament names are fully supported.
- **Weight**: Displays `remaining / total` weight.
  - Remaining weight is rounded to **0.1g**.
  - Total weight is rounded to **1g** and includes thousands separators.
  - Example: `123.4g / 1,000g`

If the Spoolman lookup fails or is not configured, these extra rows will simply be hidden, and you will only see the standard OpenSpool tag data.
