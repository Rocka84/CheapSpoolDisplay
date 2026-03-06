#include "ConfigManager.h"
#include <cstdint>

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

#ifndef USE_SDL2
uint8_t ConfigManager::getNumTools() {
  if (!preferences.isKey("num_tools")) {
    return 1; // Default to 1 tool
  }
  return preferences.getUChar("num_tools", 1);
}

void ConfigManager::setNumTools(uint8_t tools) {
  if (tools < 1)
    tools = 1;
  if (tools > 6)
    tools = 6;
  preferences.putUChar("num_tools", tools);
}

uint16_t ConfigManager::getScreenTimeout() {
  if (!preferences.isKey("timeout")) {
    return 60; // Default to 60 seconds
  }
  return preferences.getUShort("timeout", 60);
}

void ConfigManager::setScreenTimeout(uint16_t seconds) {
  if (seconds < 10)
    seconds = 10;
  if (seconds > 3600)
    seconds = 3600;
  preferences.putUShort("timeout", seconds);
}
#endif
