#include "config/ConfigManager.h"
#include "data/OpenSpool.h"
#include "nfc/NFCReader.h"
#include "power/PowerManager.h"
#include "serial/SerialTerminal.h"
#include "ui/DisplayUI.h"
#include <Arduino.h>

enum AppState { STATE_SCANNING, STATE_SHOW_INFO, STATE_EDITING };

AppState currentState = STATE_SCANNING;
OpenSpoolData currentSpoolData;

// Timer for returning to scan screen
unsigned long lastScanTime = 0;
const unsigned long INFO_TIMEOUT_MS = 30000; // Go back to scan after 30 seconds

void setup() {
  Serial.begin(115200);
  Serial.println("Starting CheapSpoolDisplay...");

  // Initialize UI (Display and LVGL)
  DisplayUI::init();

  // Initialize NFC
  NFCReader::init();

  // Initialize Power Manager
  PowerManager::init();

  // Initialize Config & Serial Terminal
  ConfigManager::init();
  SerialTerminal::init();

  // Set initial state
  currentState = STATE_SCANNING;
  DisplayUI::showScanScreen();
}

void loop() {
  // Process Serial input
  SerialTerminal::tick();

  // Let LVGL handle UI tasks
  DisplayUI::tick();

  // Power Manager tick
  PowerManager::tick();

  // If display is off and screen gets touched, wake it up
  if (PowerManager::isDisplayOff() && ts.touched()) {
    PowerManager::wakeDisplay();
    delay(500); // Debounce touch
  }

  // Reset power manager idle timer on touch
  if (ts.touched()) {
    PowerManager::resetIdleTimer();
  }

  switch (currentState) {
  case STATE_SCANNING: {
    if (NFCReader::scanForTag(currentSpoolData)) {
      PowerManager::resetIdleTimer();
      Serial.println("Tag read successfully!");
      Serial.printf(
          "Brand: %s, Type: %s, Color: %s\n", currentSpoolData.brand.c_str(),
          currentSpoolData.type.c_str(), currentSpoolData.color_hex.c_str());

      DisplayUI::showInfoScreen(currentSpoolData.brand.c_str(),
                                currentSpoolData.type.c_str(),
                                currentSpoolData.color_hex.c_str(),
                                currentSpoolData.spool_id.c_str());

      currentState = STATE_SHOW_INFO;
      lastScanTime = millis();
    }
    break;
  }

  case STATE_SHOW_INFO: {
    // Auto timeout back to scanning mode
    if (millis() - lastScanTime > INFO_TIMEOUT_MS) {
      currentState = STATE_SCANNING;
      DisplayUI::showScanScreen();
    }

    // Note: The UI "Back" button should also trigger state change.
    // Since we tied it to immediately shift screens in LVGL callbacks,
    // we will need to poll a state flag from DisplayUI or have a callback
    // to update `currentState`. For simplicity here, we assume the UI handles
    // it or we add a reset mechanism later.
    break;
  }

  case STATE_EDITING: {
    // Wait for user to save or cancel (handled via UI events)
    break;
  }
  }

  delay(5);
}
