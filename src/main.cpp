#include "data/OpenSpool.h"
#include "ui/DisplayUI.h"

#ifndef USE_SDL2
#include "config/ConfigManager.h"
#include "nfc/NFCReader.h"
#include "power/PowerManager.h"
#include "serial/SerialTerminal.h"
#include <Arduino.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

uint32_t millis() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void delay(uint32_t ms) { usleep(ms * 1000); }
#endif

enum AppState { STATE_SCANNING, STATE_SHOW_INFO, STATE_EDITING };

AppState currentState = STATE_SCANNING;
OpenSpoolData currentSpoolData;

// Timer for returning to scan screen
unsigned long lastScanTime = 0;
const unsigned long INFO_TIMEOUT_MS = 30000; // Go back to scan after 30 seconds

void setup() {
#ifndef USE_SDL2
  Serial.begin(115200);
  Serial.println("Starting CheapSpoolDisplay...");
#else
  printf("Starting CheapSpoolDisplay Simulator...\n");
#endif

  // Initialize UI (Display and LVGL)
  DisplayUI::init();

#ifndef USE_SDL2
  // Initialize NFC
  NFCReader::init();

  // Initialize Power Manager
  PowerManager::init();

  // Initialize Config & Serial Terminal
  ConfigManager::init();
  SerialTerminal::init();
#endif

  // Set initial state
  currentState = STATE_SCANNING;
  DisplayUI::showScanScreen();
}

void loop() {
#ifndef USE_SDL2
  // Process Serial input
  SerialTerminal::tick();
#endif

  // Let LVGL handle UI tasks
  DisplayUI::tick();

#ifndef USE_SDL2
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
#endif

  switch (currentState) {
  case STATE_SCANNING: {
#ifndef USE_SDL2
    if (NFCReader::scanForTag(currentSpoolData)) {
      PowerManager::resetIdleTimer();
      Serial.println("Tag read successfully!");
      Serial.printf(
          "Brand: %s, Type: %s, Color: %s\n", currentSpoolData.brand.c_str(),
          currentSpoolData.type.c_str(), currentSpoolData.color_hex.c_str());
#else
    static time_t lastModTime = 0;
    struct stat fileStat;
    bool mockRead = false;

    // Check if mock_spool.json was updated
    if (stat("mock_spool.json", &fileStat) == 0) {
      if (fileStat.st_mtime > lastModTime) {
        // Updated or first read
        if (lastModTime == 0) {
          printf("Found mock_spool.json. Reading mock tag...\n");
        } else {
          printf("mock_spool.json changed. Triggering new mock tag read...\n");
        }
        lastModTime = fileStat.st_mtime;

        FILE *fp = fopen("mock_spool.json", "r");
        if (fp) {
          fseek(fp, 0, SEEK_END);
          long size = ftell(fp);
          fseek(fp, 0, SEEK_SET);

          if (size > 0) {
            char *buffer = (char *)malloc(size + 1);
            if (buffer) {
              size_t bytesRead = fread(buffer, 1, size, fp);
              buffer[bytesRead] = '\0';
              std::string jsonStr(buffer);

              if (OpenSpoolParser::parseJson(jsonStr, currentSpoolData)) {
                mockRead = true;
              } else {
                printf("Error parsing mock_spool.json!\n");
              }
              free(buffer);
            }
          }
          fclose(fp);
        }
      }
    }

    if (mockRead) {
#endif

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

#ifdef USE_SDL2
int main(int argc, char **argv) {
  setup();
  while (1) {
    loop();
  }
  return 0;
}
#endif
