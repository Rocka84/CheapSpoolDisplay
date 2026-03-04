#include "PowerManager.h"
#include <esp_sleep.h>

// Assuming we can read battery voltage somewhere, e.g., on some CYD models
// pin 34 is connected to a divider or LDR that we can hijack, or we just rely
// on a simple defined constant if the user configures it.
// For now, many CYD boards have a rudimentary divider on GPIO 34 (LDR).
// If LDR reading is high, it might be USB. If low, battery. We will use a
// placeholder logic here that can be calibrated.
#define BATTERY_SENSE_PIN 34

unsigned long PowerManager::lastActivityTime = 0;
bool PowerManager::displayIsOff = false;

// TFT Backlight pin defined in platformio.ini as TFT_BL (21)
#ifndef TFT_BL
#define TFT_BL 21
#endif

void PowerManager::init() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // Display ON
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
  digitalWrite(TFT_BL, HIGH);
  displayIsOff = false;
  Serial.println("Display Woke Up");
}

void PowerManager::turnDisplayOff() {
  digitalWrite(TFT_BL, LOW);
  displayIsOff = true;
  Serial.println("Display Turned Off (USB Power)");
}

void PowerManager::enterDeepSleep() {
  Serial.println("Entering Deep Sleep (Battery Power)");
  digitalWrite(TFT_BL, LOW);

  // Configure wake up source: EXT0 on Reset button (EN/RST pin handles reboot
  // anyway) Or if we want EXT0 on another pin, we can set it here. For CYD,
  // pressing RESET physically restarts the ESP32, which wakes it from deep
  // sleep. So we just go to sleep forever.
  esp_deep_sleep_start();
}

bool PowerManager::isPoweredByUSB() {
  // See HARDWARE_SETUP.md for mod details.
  // Assuming a 22k/10k voltage divider on the BATTERY_SENSE_PIN (IO34)
  // connected to V-In. 5V USB source -> ~1.56V at ADC 4.2V LiPo source ->
  // ~1.31V at ADC 3.7V LiPo source -> ~1.15V at ADC ESP32 ADC maxes at ~3.3V
  // (4095). 1.56V is roughly ADC value 1935. 1.31V is roughly ADC value 1625.
  // Threshold set to 1800 to differentiate USB (5V) and Battery (<4.2V)

  int val = analogRead(BATTERY_SENSE_PIN);
  Serial.printf("Power Sense ADC: %d\n", val);

  return (val > 1800);
}

void PowerManager::tick() {
  if (displayIsOff)
    return; // Wait for external wake (e.g. touch interrupt)

  if (millis() - lastActivityTime > IDLE_TIMEOUT_MS) {
    if (isPoweredByUSB()) {
      turnDisplayOff();
    } else {
      enterDeepSleep();
    }
  }
}
