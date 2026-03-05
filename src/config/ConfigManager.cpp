#include "ConfigManager.h"

Preferences ConfigManager::preferences;

void ConfigManager::init() {
  // Open Preferences with my-app namespace. Each application module, library,
  // etc has to use a namespace name to prevent key name collisions. We will
  // open storage in RW-mode (second parameter has to be false).
  preferences.begin("csd-config", false);
}

std::string ConfigManager::getWifiSSID() {
  return std::string(preferences.getString("ssid", "").c_str());
}

void ConfigManager::setWifiSSID(const std::string &ssid) {
  preferences.putString("ssid", ssid.c_str());
}

std::string ConfigManager::getWifiPass() {
  return std::string(preferences.getString("pass", "").c_str());
}

void ConfigManager::setWifiPass(const std::string &pass) {
  preferences.putString("pass", pass.c_str());
}

std::string ConfigManager::getWebhookUrl() {
  return std::string(preferences.getString("wh_url", "").c_str());
}

void ConfigManager::setWebhookUrl(const std::string &url) {
  preferences.putString("wh_url", url.c_str());
}
