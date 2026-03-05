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

std::string ConfigManager::getWebhook() {
#ifndef USE_SDL2
  // We keep "wh_url" as the NVS key for backward compatibility or rename it to
  // "webhook" User asked to call it "webhook" instead of "url". I will use
  // "webhook" as the NVS key now for consistency with the new command name.
  if (!preferences.isKey("webhook")) {
    return "";
  }
  return std::string(preferences.getString("webhook", "").c_str());
#else
  return "http://localhost:8080/webhook?spool={spool_id}&tool={toolhead}";
#endif
}

void ConfigManager::setWebhook(const std::string &url) {
#ifndef USE_SDL2
  preferences.putString("webhook", url.c_str());
#endif
}
