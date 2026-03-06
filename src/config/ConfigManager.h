#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H
#ifndef USE_SDL2
#include <Arduino.h>
#include <Preferences.h>
#endif
#include <string>

#ifdef USE_SDL2
#include <stdint.h>
#endif

class ConfigManager {
public:
  static void init();

  // Wi-Fi Settings
  static std::string getWifiSSID();
  static void setWifiSSID(const std::string &ssid);

  static std::string getWifiPass();
  static void setWifiPass(const std::string &pass);

  static std::string getWebhook();
  static void setWebhook(const std::string &url);

#ifndef USE_SDL2
  static uint8_t getNumTools();
  static void setNumTools(uint8_t tools);

  static uint16_t getScreenTimeout();
  static void setScreenTimeout(uint16_t seconds);
#else
  static uint8_t getNumTools() { return 4; }
  static void setNumTools(uint8_t tools) {}

  static uint16_t getScreenTimeout() { return 60; }
  static void setScreenTimeout(uint16_t seconds) {}
#endif

private:
#ifndef USE_SDL2
  static Preferences preferences;
#endif
};

#endif // CONFIG_MANAGER_H
