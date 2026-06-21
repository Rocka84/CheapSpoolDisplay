# AI Agent Project Overview: CheapSpoolDisplay

This document is specifically written for AI Coding Assistants (like Antigravity) to quickly familiarize themselves with the architecture, tools, and conventions of the **CheapSpoolDisplay** project.

## 1. Project Summary
CheapSpoolDisplay is a versatile C++ firmware for the **ESP32 Cheap Yellow Display (CYD)**. Its primary purpose is to scan, view, and organize 3D printer filament spools using various NFC/RFID tag standards (OpenSpool, OpenPrintTag, OpenTag3D, Snapmaker, Bambu Lab). 
It features a modern touch UI, integrates with **Spoolman** over WiFi, and can directly interface with **Snapmaker U1** printers (via the Extended Firmware) to act as an external filament scanner.

## 2. Technology Stack & Frameworks
- **Environment:** PlatformIO (PIO)
- **Microcontroller:** ESP32 (CYD)
- **UI Framework:** LVGL v9.1.0 (with TFT_eSPI for the display driver)
- **RFID/NFC Hardware:** MFRC522 (ISO14443A) or PN5180 (ISO15693/ICODE)
- **Libraries:** ArduinoJson (JSON parsing), tinycbor (CBOR encoding for tags)

## 3. PIO Environments (`platformio.ini`)
Always use the correct environment when compiling or running tasks:
- `esp32dev`: Default build for MFRC522 hardware.
- `pn5180`: Build for PN5180 hardware.
- `simulator`: Native SDL2 build to test the UI on the desktop without hardware.
- `desktop`: Native unit testing environment.
- `test_embedded`: Hardware-in-the-loop integration tests.

## 4. The Desktop Simulator (Crucial for UI Dev)
You do **not** need to flash the ESP32 to test UI changes! The project includes a native SDL2 simulator.
- **To build and run:**
  ```bash
  pio run -e simulator
  ./simulator/program
  ```
- **How it works:** It opens a native window on the desktop. It is **not** a web app and requires no browser or VNC.
- **Screenshots:** If you need to verify visual changes, take a screenshot by running `./simulator/screenshot.sh` while the simulator is running.
- **Rule:** **Always** rebuild both the main project and the simulator after making changes to ensure compatibility across both targets.

## 5. Project Structure
- `src/ui/` - LVGL screens, widgets, and display initialization.
- `src/nfc/` - Hardware abstraction for MFRC522 and PN5180.
- `src/data/` - Tag format parsers and encoders (OpenSpool, OpenPrintTag, Snapmaker, Bambu, etc.).
- `src/network/` - WiFi setup, Spoolman API clients, and Webhook/U1 HTTP request handling.
- `src/serial/` - CLI interface via the USB serial port for device configuration.
- `src/power/` - Battery voltage monitoring (ADC) and sleep management heuristics.
- `docs/` - User-facing documentation and hardware setup guides. **This is the source of truth.** Do not edit `web/docs/` directly.
- `scripts/` - Python pre/post scripts for PlatformIO, and bash scripts like `copy_docs.sh` to sync docs to the `web/` folder.
- `simulator/` - Mock data and configs for the native UI simulator.

## 6. Global Rules & Code Conventions
- **Language:** Always use **English** for code, variable names, and comments, regardless of the language the user is speaking with you.
- **Comments:** Keep comments **sparse** and only document non-obvious logic. Avoid redundant or overly verbose comments.
- **Non-Volatile Storage (NVS):** Configuration (WiFi, Spoolman URL, Webhook, Tag Format) is stored in NVS and managed via the `ConfigManager`. Use the `serial` CLI to mutate these during runtime.
- **Memory:** ESP32 has limited RAM. Be careful with large JSON allocations (use `JsonDocument` carefully) and LVGL memory usage.

## 7. Current State & Notes
- Check the `notes.md` file in the project root. It contains the user's running scratchpad of ideas, known issues, and ToDo lists.
- **Bambu Lab & Snapmaker Proprietary Tags:** Read-only support exists. Writing requires reverse-engineering or keys that are not public. Bambu Lab needs a secret salt configured via CLI (`set bambu_salt`).
- **Power Management:** Custom heuristics map 3.3V-4.15V ADC readings to battery percentages, accounting for active load voltage sag.
- **Spoolman Tag Linking:** We no longer generate fake `lot_nr` values for proprietary tags. All physical tags are native tracked in Spoolman via the `extra.card_uids` field (mapped to the physical `hardware_uid`). The UI features an interactive "Link Spool" button when an unknown tag is scanned to associate it easily.

## 8. Agentic Workflow
1. Read the user's prompt carefully.
2. If UI changes are requested, modify `src/ui/`, build the `simulator` environment, and run it to verify.
3. **Documentation:** The files in `docs/` and the root `README.md` are the source of truth. If you edit them, you MUST run `./scripts/copy_docs.sh` to update the `web/` directory. Do not edit `web/docs/` or `web/README.md` directly.
4. Keep `notes.md` updated if you complete tasks or find new known issues.
5. Keep `AGENTS.md` updated if the project architecture, dependencies, hardware integrations, or core guidelines change.
6. If a task involves complex logic, use your `brainstorming` skill before writing code.
