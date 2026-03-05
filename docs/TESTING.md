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

