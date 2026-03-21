#include "PowerManager.h"
#include "../config/ConfigManager.h"
#include <esp_sleep.h>

// Assuming we can read battery voltage somewhere, e.g., on some CYD models
// pin 34 is connected to a divider or LDR that we can hijack, or we just rely
// on a simple defined constant if the user configures it.
// To avoid physically modifying the CYD hardware (like desoldering the LDR on
// IO34), we use IO35, which is purely an ADC input pin physically broken out on
// the CYD's P3/CN1 external connector.
#define BATTERY_SENSE_PIN 35

unsigned long PowerManager::lastActivityTime = 0;
bool PowerManager::displayIsOff = false;
unsigned long PowerManager::buttonPressStart = 0;

#define SLEEP_BUTTON_PIN 0
#define SLEEP_HOLD_MS 2000

// TFT Backlight pin defined in platformio.ini as TFT_BL (21)
#ifndef TFT_BL
#define TFT_BL 21
#endif

void PowerManager::init() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(SLEEP_BUTTON_PIN, INPUT_PULLUP);
  lastActivityTime = millis();
  displayIsOff = false;
}

void PowerManager::resetIdleTimer() {
  lastActivityTime = millis();
  if (displayIsOff) {
    wakeDisplay();
  }
}

bool PowerManager::isDisplayOff() { return displayIsOff; }

void PowerManager::wakeDisplay() {
  lastActivityTime =
      millis(); // Reset timer so we don't immediately sleep again
  digitalWrite(TFT_BL, HIGH);
  displayIsOff = false;
}

void PowerManager::turnDisplayOff() {
  digitalWrite(TFT_BL, LOW);
  displayIsOff = true;
}

void PowerManager::enterDeepSleep() {
  digitalWrite(TFT_BL, LOW);

  // Configure wake up source: EXT0 on Reset button (EN/RST pin handles reboot
  // anyway) Or if we want EXT0 on another pin, we can set it here. For CYD,
  // pressing RESET physically restarts the ESP32, which wakes it from deep
  // sleep. So we just go to sleep forever.
  esp_deep_sleep_start();
}

bool PowerManager::isPoweredByUSB() {
  // Battery feature postponed. Assume USB power always.
  return true;
}

void PowerManager::tick() {
  if (digitalRead(SLEEP_BUTTON_PIN) == LOW) {
    if (buttonPressStart == 0) buttonPressStart = millis();
    else if (millis() - buttonPressStart >= SLEEP_HOLD_MS) enterDeepSleep();
  } else {
    buttonPressStart = 0;
  }

  if (displayIsOff)
    return;

  if (millis() - lastActivityTime >
      (ConfigManager::getScreenTimeout() * 1000UL)) {
    if (isPoweredByUSB()) {
      turnDisplayOff();
    } else {
      enterDeepSleep();
    }
  }
}
