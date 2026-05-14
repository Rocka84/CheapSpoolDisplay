# Configuration

CheapSpoolDisplay can be configured via a Serial Terminal (115200 baud) when connected to your computer via USB.

`pio device monitor`

## Serial Terminal Commands

| Command | Description |
| :--- | :--- |
| `help` | Show available commands |
| `get config` | Show current configuration values |
| `set wifi <ssid> <password>` | Special command to set Wi-Fi credentials |
| `set spoolman http://<ip>:<port>` | Set the Spoolman server URL |
| `set webhook http://...` | Set the webhook URL |
| `set u1_host <ip>:<port>` | Set the Snapmaker U1 host URL |
| `set tag_format <openspool\|openprinttag\|opentag3d\|ask>` | Set the default NFC writing format (Default: ask) |
| `set bambu_salt <hex_string>` | Set the Secret Salt for Bambu Lab tags |
| `set wifi_timeout <sec>` | Set the maximum wait time for Wi-Fi connection (10-300) |
| `set tools <1-16>` | Set the number of toolheads |
| `set display_timeout <sec>` | Set the screen auto-off time (0 = always on) |
| `set sleep_timeout <sec>` | Set the idle time before deep sleep |
| `set power_mode <0\|1\|2>` | Set the power behavior (0=Always On, 1=Deep Sleep, 2=Smart USB) |
| `set cyd2usb <0\|1>` | Fix inverted colors/gamma for 2-USB boards |
| `format` | Erases all configuration and resets to defaults |

## CYD hardware versions
If your display has two USB ports (one micro-USB and one USB-C), colors probably appear inverted (black appears white, etc.). To fix this:

1. Connect via Serial Terminal.
2. Run: `set cyd2usb 1`
3. Restart the device.

This setting also applies a specific gamma correction table to fix the washed-out look typical of these newer panels.

## Configuration Keys

### Wi-Fi

`set wifi SSID PASSWORD`

**Note:** The device will only connect to Wi-Fi when needed (e.g., for Spoolman or Webhooks). You can optionally configure the connection timeout limit using:
`set wifi_timeout <seconds>` (Default: 60)

### Spoolman Integration

`set spoolman http://<ip>:<port>`

Enables data enrichment (Filament Name and Weight) for spools with a `spool_id` on the tag.

### Webhooks

`set webhook http://...`

A URL that will be called (POST) whenever a spool is detected. You can use placeholders like `{spool_id}` or `{toolhead}` in the URL to trigger GET mode.

### Snapmaker U1 Integration

`set u1_host <ip>:<port>`

Configures the address of your printer's Moonraker API (default port is usually 7125). This allows the device to send filament detection events directly to the printer.

### Number of Tools

`set tools <1-16>`

Configures how many toolheads your printer has. Up to 4 tools result in a fixed layout; more than 4 will use a scrollable 2-column grid.

### Tag Format

`set tag_format <openspool|openprinttag|opentag3d|ask>`

Defines the default protocol used when writing or creating new NFC tags:
- `openspool`: Standard format.
- `openprinttag`: Efficient binary format.
- `opentag3d`: Compact format.
- `ask`: Shows a selection dialog on the device every time you save. (Default)

### Bambu Lab Support

`set bambu_salt <hex_string>`

To enable read-only support for authentic Bambu Lab filament tags, you must provide the Secret Salt used for key derivation.

> [!IMPORTANT]
> For legal reasons, this salt is not included in the firmware. You must obtain it from community resources (e.g., search for "Bambu Lab RFID secret salt").

### Power & Display Management

**Display Timeout:**
`set display_timeout <seconds>`
Configures how long the screen remains on when idle. Use `0` to keep the screen on forever.

**Power Mode:**
`set power_mode <0|1|2>`
Configures the battery and deep sleep behavior:
- `0`: **Always On** (never goes to deep sleep, display timeout still applies)
- `1`: **Deep Sleep** (enters deep sleep after idle time)
- `2`: **Smart USB** (stays awake when USB power is detected, deep sleeps when on battery)

**Sleep Timeout:**
`set sleep_timeout <seconds>`
Configures how many seconds the device must be idle before entering deep sleep (only applies to Power Modes 1 and 2). Valid range: 60 to 3600.

## Examples

```bash
# Set Wi-Fi
set wifi MySSID MySecretPassword

# Configure Spoolman
set spoolman http://192.168.1.50:8000

# Configure Snapmaker U1
set u1_host 192.168.1.70

# Set 16 tools instead of 1
set tools 16

# Enable 5 minute screen timeout (300 seconds)
set display_timeout 300

# Enable Smart USB mode with a 15-minute sleep timeout (900 seconds)
set power_mode 2
set sleep_timeout 900
```
