#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <string>

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

private:
  static Preferences preferences;
};

#endif // CONFIG_MANAGER_H
