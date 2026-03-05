# Web Installer Local Development Guide

To test the web installer locally, you must serve it over HTTP (opening `index.html` directly will disable USB access).

## 1. Build the Firmware Binaries
Compile the project to generate the required binaries in `.pio/build/esp32dev/`:
```bash
pio run -e esp32dev
```

## 2. Start a Local HTTP Server
Run this from the **root** of the repository:
```bash
python -m http.server 8000
```

## 3. Flash the Device
1. Open Google Chrome or Microsoft Edge.
2. Navigate to: `http://localhost:8000/web/index.html`
3. Connect your CYD board via USB.
4. Click "Connect & Flash Device".
