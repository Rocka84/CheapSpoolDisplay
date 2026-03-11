# Testing the CheapSpoolDisplay Firmware

This project uses PlatformIO's built-in advanced testing framework (`Unity`) to assure code quality, parser safety, and hardware memory integrity. The tests are divided into two distinct environments to speed up development.

## 1. Desktop Tests (No Hardware Required)

Desktop tests compile and run entirely on your desktop PC. They use a standard `gcc`/`g++` compiler, mock out hardware-specific dependencies, and completely isolate the core algorithm logic. This means they run in milliseconds, making them perfect for CI/CD or rapid iteration.

### Running Desktop Tests
You can execute these at any time (even if the CYD board is unplugged!):
```bash
pio test -e desktop
```

### What is tested locally?
*   **`test_desktop/test_main.cpp` (OpenSpool Data Structures):** 
    Tests the serialization and deserialization of the JSON-based OpenSpool specification using the `ArduinoJson` library. It verifies that valid JSON strings populate the correct memory struct fields (`color_hex`, `spool_id`), and conversely, that missing fields don't cause fatal crashes.
*   **`test_desktop_serial/test_serialparser.cpp`:**
    An isolated unit test of the string extraction logic derived from `SerialTerminal.cpp`. It specifically feeds malformed strings, strings with excess spaces, and partial keys into the parser to ensure the core string-trimming operations work correctly without crashing or splitting incorrectly.

---

## 2. Embedded Hardware Tests (CYD Must Be Connected via USB)

Embedded tests are written against the actual ESP32 silicon. PlatformIO compiles the test framework alongside the project firmware, uploads it over USB, and monitors the Serial output to report on assertion passes/failures. **These require the device to be physically plugged into your PC.**

### Running Embedded Tests
```bash
# General run command (Make sure no other Serial Monitors are active)
pio test -e test_embedded
```

### What is tested embedded?
*   **`test_embedded/test_nvs.cpp` (Non-Volatile Storage):**
    These tests verify the core reliability of the ESP32's flash memory via the `Preferences.h` API used by `ConfigManager`. The test suite automatically writes dummy Wi-Fi passwords and Webhook URLs into flash storage, reads them back into RAM, asserts they match character-for-character, and finally invokes the `format()` sequence to leave the device clean. 

## Troubleshooting Tests

-   **"Uploading stage has failed," or "Error 1":** Ensure the device is plugged in using a data-capable USB cord and that you do not have another Serial terminal or Web Installer actively grabbing the COM port.
-   **Linker Errors or missing definitions:** Ensure you are using the correct environment. Board tests MUST use `-e test_embedded` to correctly include the project source code while excluding the main application UI and logic.
```bash
pio test -e test_embedded
```

---

## 3. UI Simulator (Premium Interface Preview)

The UI simulator allows you to run the entire LVGL application on your desktop. It uses the `native` PlatformIO environment and `SDL2` to create a window that mimics the 240x320 CYD screen.

### Desktop Dependencies
Before running the simulator, you must have the **SDL2** development libraries installed:

- **Linux (Debian/Ubuntu)**: `sudo apt-get install libsdl2-dev`
- **macOS**: `brew install sdl2`
- **Windows**: Use MSYS2 or download the development headers from the [SDL2 website](https://www.libsdl.org/download-2.0.php).

### Running the Simulator
To build and launch the simulator window:
```bash
pio run -e simulator
./.pio/build/simulator/program
```
*Note: The simulator skips hardware-specific code (NFC, Wi-Fi, NVS) and simulates the Premium UI exactly as it appears on the device.*

---

## 4. Webhook Test Server

For local development and testing of the webhook integration, a simple Python-based test server is provided in the `scripts/` directory.

### Running the Test Server
1.  **Start the server**:
    ```bash
    ./scripts/test_webhook_server.py
    ```
    The server will start listening on port **7125** (default for Moonraker/U1).

2.  **Configure the device**:
    -   Find your computer's local IP address.
    -   Set the webhook URL on the device or via the serial terminal:
        ```bash
        set webhook http://<your-ip>:7125/
        ```

3.  **Trigger a webhook**:
    Scan a spool tag. The test server will log the incoming request details, including:
    -   **HTTP Method** (GET or POST)
    -   **Full Headers**
    -   **JSON Payload** (for POST) or **URL Query Parameters** (for GET)

### Why use this?
-   **No cloud setup**: Test your webhook logic without needing a public endpoint or Spoolman instance.
-   **Debugging**: See exactly what the device is sending, including content-type and payload structure.
-   **Simulator support**: The simulator also sends webhooks, allowing for a fully virtual testing loop.

---

## 5. Snapmaker U1 Integration Testing

The Snapmaker U1 integration uses a direct POST to `/printer/filament_detect/set`. You can verify this using the simulator and the provided test server.

### Verification Steps
1.  **Start the Test Server**:
    ```bash
    ./scripts/test_webhook_server.py
    ```
    *(Defaults to port 7125).*

2.  **Configure the Simulator**:
    -   Open `simulator/config.cfg`.
    -   Set `u1_host` to `localhost:7125`.

3.  **Run the Simulator**:
    ```bash
    ./simulator/program
    ```

4.  **Load Spool**:
    -   Scan a mock spool (by updating `simulator/spool.json`).
    -   On the **Info Screen**, click the **Load** button.
    -   If prompted, select a toolhead/channel.

5.  **Check Results**:
    Observe the test server terminal output. You should see a log entry for `[U1 API] Detected Direct Filament Set!` containing the structured "OpenSpool U1 Extended Format" JSON payload.

### Troubleshooting U1 Integration
-   **No request received**: Ensure `u1_host` is correctly set and the test server is running on the same port.
-   **Incorrect fields**: Check the logic in `NetworkManager::sendWebhookPayload`. All mapped fields (`VENDOR`, `MAIN_TYPE`, `RGB_1`, etc.) must match the specification in `filament_detect.md`.

