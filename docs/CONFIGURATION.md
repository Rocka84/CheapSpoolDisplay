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
| `set tools <1-16>` | Set the number of toolheads |
| `set display_timeout <sec>` | Set the screen auto-off time (0 = always on) |
| `set sleep_timeout <min>` | Set the idle time before deep sleep |
| `set power_mode <0\|1\|2>` | Set the power behavior (0=Always On, 1=Deep Sleep, 2=Smart USB) |
| `format` | Erases all configuration and resets to defaults |

## Configuration Keys

### Wi-Fi

`set wifi SSID PASSWORD`

**Note:** The device will only connect to Wi-Fi when needed (e.g., for Spoolman or Webhooks).

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
`set sleep_timeout <minutes>`
Configures how many minutes the device must be idle before entering deep sleep (only applies to Power Modes 1 and 2). Valid range: 1 to 60.

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

# Enable 5 minute screen timeout
set display_timeout 300

# Enable Smart USB mode with a 15-minute sleep timeout
set power_mode 2
set sleep_timeout 15
```
