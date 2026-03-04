#ifndef POWER_MGR_H
#define POWER_MGR_H

#include <Arduino.h>

class PowerManager {
public:
  static void init();
  static void tick();
  static void resetIdleTimer();
  static bool isDisplayOff();
  static void wakeDisplay();

private:
  static bool isPoweredByUSB();
  static void turnDisplayOff();
  static void enterDeepSleep();

  static unsigned long lastActivityTime;
  static const unsigned long IDLE_TIMEOUT_MS = 60000; // 60 seconds
  static bool displayIsOff;
};

#endif // POWER_MGR_H
