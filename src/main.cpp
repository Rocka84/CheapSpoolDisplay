#include "data/OpenSpool.h"
#include "ui/DisplayUI.h"

#include "config/ConfigManager.h"
#include "network/NetworkManager.h"

#ifndef USE_SDL2
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

  // Initialize Config
  ConfigManager::init();

#ifndef USE_SDL2
  // Initialize NFC
  NFCReader::init();

  // Initialize Power Manager
  PowerManager::init();

  // Initialize Serial Terminal
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
  if (PowerManager::isDisplayOff() && ts.getTouch().zRaw > 0) {
    PowerManager::wakeDisplay();
    delay(500); // Debounce touch
  }

  // Reset power manager idle timer on touch
  if (ts.getTouch().zRaw > 0) {
    PowerManager::resetIdleTimer();
  }
#endif

  // Check for scanning in both SCANNING and SHOW_INFO states
  bool tagScanned = false;
  if (currentState == STATE_SCANNING || currentState == STATE_SHOW_INFO) {
#ifndef USE_SDL2
    if (NFCReader::scanForTag(currentSpoolData)) {
      PowerManager::resetIdleTimer();
      Serial.println("Tag read successfully!");
      Serial.printf(
          "Brand: %s, Type: %s, Color: %s\n", currentSpoolData.brand.c_str(),
          currentSpoolData.type.c_str(), currentSpoolData.color_hex.c_str());
      tagScanned = true;
    }
#else
    static time_t lastModTime = 0;
    struct stat fileStat;

    // Check if simulator/spool.json was updated
    if (stat("simulator/spool.json", &fileStat) == 0) {
      if (fileStat.st_mtime > lastModTime) {
        // Updated or first read
        if (lastModTime == 0) {
          printf("Found simulator/spool.json. Reading mock tag...\n");
        } else {
          printf("simulator/spool.json changed. Triggering new mock tag "
                 "read...\n");
        }
        lastModTime = fileStat.st_mtime;

        FILE *fp = fopen("simulator/spool.json", "r");
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
                tagScanned = true;
              } else {
                printf("Error parsing simulator/spool.json!\n");
              }
              free(buffer);
            }
          }
          fclose(fp);
        }
      }
    }
#endif
  }

  // If a tag was scanned, we always transition to or refresh the Info Screen
  if (tagScanned) {
    // Enrich with Spoolman data if configured
    if (!ConfigManager::getSpoolmanUrl().empty() &&
        !currentSpoolData.spool_id.empty()) {
#ifndef USE_SDL2
      Serial.println(
          "Main: Spoolman URL configured, attempting data enrichment...");
#else
      printf("Main: Spoolman URL configured (%s), attempting data enrichment "
             "for spool %s...\n",
             ConfigManager::getSpoolmanUrl().c_str(),
             currentSpoolData.spool_id.c_str());
#endif
      if (NetworkManager::fetchSpoolmanData(currentSpoolData)) {
#ifdef USE_SDL2
        printf("Main: Spoolman enrichment successful for %s\n",
               currentSpoolData.filament_name.c_str());
#endif
      } else {
#ifdef USE_SDL2
        printf("Main: Spoolman enrichment failed.\n");
#endif
      }
    }

    DisplayUI::showInfoScreen(currentSpoolData);

    currentState = STATE_SHOW_INFO;
    lastScanTime = millis();
  }

  switch (currentState) {
  case STATE_SCANNING: {
    // Nothing extra to do here, background scanning is handled above
    break;
  }

  case STATE_SHOW_INFO: {
    // Auto timeout back to scanning mode, or manual return via UI back button
    if (millis() - lastScanTime > INFO_TIMEOUT_MS ||
        DisplayUI::isScanScreenActive()) {
      currentState = STATE_SCANNING;
      if (!DisplayUI::isScanScreenActive()) {
        DisplayUI::showScanScreen();
      }
    }

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
