#include "ConfigManager.h"

#ifndef USE_SDL2
Preferences ConfigManager::preferences;

void ConfigManager::init() { preferences.begin("csd-config", false); }
#else
void ConfigManager::init() {
  // Mock init for simulator
}
#endif

std::string ConfigManager::getWifiSSID() {
#ifndef USE_SDL2
  if (!preferences.isKey("ssid"))
    return "";
  return std::string(preferences.getString("ssid", "").c_str());
#else
  return "Simulator_WiFi";
#endif
}

void ConfigManager::setWifiSSID(const std::string &ssid) {
#ifndef USE_SDL2
  preferences.putString("ssid", ssid.c_str());
#endif
}

std::string ConfigManager::getWifiPass() {
#ifndef USE_SDL2
  if (!preferences.isKey("pass"))
    return "";
  return std::string(preferences.getString("pass", "").c_str());
#else
  return "simulator_pass";
#endif
}

void ConfigManager::setWifiPass(const std::string &pass) {
#ifndef USE_SDL2
  preferences.putString("pass", pass.c_str());
#endif
}

std::string ConfigManager::getWebhookUrl() {
#ifndef USE_SDL2
  if (!preferences.isKey("wh_url"))
    return "";
  return std::string(preferences.getString("wh_url", "").c_str());
#else
  return "http://localhost:8080/webhook?spool={spool_id}&tool={toolhead}";
#endif
}

void ConfigManager::setWebhookUrl(const std::string &url) {
#ifndef USE_SDL2
  preferences.putString("wh_url", url.c_str());
#endif
}
