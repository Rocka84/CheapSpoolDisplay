#include "PowerManager.h"
#include "../config/ConfigManager.h"
#include <esp_sleep.h>

// Assuming we can read battery voltage somewhere, e.g., on some CYD models
// pin 34 is connected to a divider or LDR that we can hijack, or we just rely
// on a simple defined constant if the user configures it.
// To avoid physically modifying the CYD hardware (like desoldering the LDR on
// IO34), we use IO35, which is purely an ADC input pin physically broken out on
// the CYD's P3/CN1 external connector.
#ifndef USE_SDL2
#define BATTERY_SENSE_PIN 35
#define ADC_USB_THRESHOLD 2600 // Roughly equates to > 4.2V on a 1/2 divider
#endif

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
#ifndef USE_SDL2
  // Simple heuristic: if ADC reads high (assuming a 1/2 voltage divider to Pin 35)
  // 4.2V / 2 = 2.1V. Full ADC range is 3.3V (4095). 2.1V = ~2600
  return analogRead(BATTERY_SENSE_PIN) > ADC_USB_THRESHOLD;
#else
  return true;
#endif
}

void PowerManager::tick() {
  if (digitalRead(SLEEP_BUTTON_PIN) == LOW) {
    if (buttonPressStart == 0) buttonPressStart = millis();
    else if (millis() - buttonPressStart >= SLEEP_HOLD_MS) enterDeepSleep();
  } else {
    buttonPressStart = 0;
  }

  uint8_t powerMode = ConfigManager::getPowerMode();
  uint16_t sleepTimeoutMins = ConfigManager::getSleepTimeout();
  uint16_t displayTimeoutSecs = ConfigManager::getDisplayTimeout();
  
  unsigned long idleTimeMs = millis() - lastActivityTime;

  // 1. Turn off display if timeout matches (and isn't disabled/0)
  if (!displayIsOff && displayTimeoutSecs > 0) {
    if (idleTimeMs > (displayTimeoutSecs * 1000UL)) {
      turnDisplayOff();
    }
  }

  // 2. Process Sleep Logic
  // Mode 0: Always On (Never sleep)
  if (powerMode == 0) return;

  // Mode 2: Smart Mode (Always On when USB, Sleep on Battery)
  if (powerMode == 2 && isPoweredByUSB()) return;

  // Mode 1 or Mode 2 (on battery): Deep sleep after idle timeout
  unsigned long sleepTimeoutMs = sleepTimeoutMins * 60000UL;
  if (idleTimeMs > sleepTimeoutMs) {
    enterDeepSleep();
  }
}
