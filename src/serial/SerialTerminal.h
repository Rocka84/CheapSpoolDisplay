#ifndef SERIAL_TERMINAL_H
#define SERIAL_TERMINAL_H

#include <Arduino.h>

class SerialTerminal {
public:
  static void init();
  static void tick(); // Call this in main loop to check for incoming chars.
private:
  static void processCommand(const String &cmd);
  static String rxBuffer;
};

#endif // SERIAL_TERMINAL_H
