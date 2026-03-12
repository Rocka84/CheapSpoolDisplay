# Configuration

CheapSpoolDisplay can be configured via a Serial Terminal (115200 baud) when connected to your computer via USB.

## Serial Terminal Commands

| Command | Description |
| :--- | :--- |
| `help` | Show available commands |
| `get config` | Show current configuration values |
| `set wifi <ssid> <password>` | Special command to set Wi-Fi credentials |
| `set spoolman http://<ip>:<port>` | Set the Spoolman server URL |
| `set webhook http://...` | Set the webhook URL |
| `set u1_host <ip>:<port>` | Set the Snapmaker U1 host URL |
| `set num_tools <1-16>` | Set the number of toolheads |
| `set timeout <seconds>` | Set the screen timeout |
| `reset config` | Reset all configuration to defaults |

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

`set num_tools <1-16>`

Configures how many toolheads your printer has. Up to 4 tools result in a fixed layout; more than 4 will use a scrollable 2-column grid.

### Screen Timeout

`set timeout <seconds>`

Configures how long the info screen remains visible before timing out back to the home screen (0 to disable).

## Examples

```bash
# Set Wi-Fi
set wifi MySSID MySecretPassword

# Configure Spoolman
set spoolman http://192.168.1.50:8000

# Configure Snapmaker U1
set u1_host 192.168.1.70

# Set 16 tools instead of 1
set num_tools 16

# Enable 5 minute screen timeout
set timeout 300
```
