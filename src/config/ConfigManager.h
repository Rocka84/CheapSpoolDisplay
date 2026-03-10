#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H
#ifndef USE_SDL2
#include <Arduino.h>
#include <Preferences.h>
#endif
#include <stdint.h>
#include <string>

#ifdef USE_SDL2
#include <map>
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

  static std::string getSpoolmanUrl();
  static void setSpoolmanUrl(const std::string &url);

  static uint8_t getNumTools();
  static void setNumTools(uint8_t tools);

  static uint16_t getScreenTimeout();
  static void setScreenTimeout(uint16_t seconds);

  // Snapmaker U1 Settings
  static std::string getU1Host();
  static void setU1Host(const std::string &host);

private:
#ifndef USE_SDL2
  static Preferences preferences;
#else
  static std::map<std::string, std::string> simConfig;
  static void loadSimConfig();
#endif
};

#endif // CONFIG_MANAGER_H
