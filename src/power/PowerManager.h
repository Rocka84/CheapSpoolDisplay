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
  static void enterDeepSleep();

private:
  static bool isPoweredByUSB();
  static void turnDisplayOff();

  static unsigned long lastActivityTime;
  static bool displayIsOff;
  static unsigned long buttonPressStart;
};

#endif // POWER_MGR_H
