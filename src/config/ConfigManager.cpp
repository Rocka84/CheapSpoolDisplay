#include "ConfigManager.h"
#include <cstdint>

#ifndef USE_SDL2
#include <Arduino.h>

Preferences ConfigManager::preferences;

void ConfigManager::init() { preferences.begin("csd-config", false); }

std::string ConfigManager::getWifiSSID() {
  if (!preferences.isKey("ssid"))
    return "";
  return std::string(preferences.getString("ssid", "").c_str());
}

void ConfigManager::setWifiSSID(const std::string &ssid) {
  preferences.putString("ssid", ssid.c_str());
}

std::string ConfigManager::getWifiPass() {
  if (!preferences.isKey("pass"))
    return "";
  return std::string(preferences.getString("pass", "").c_str());
}

void ConfigManager::setWifiPass(const std::string &pass) {
  preferences.putString("pass", pass.c_str());
}

int ConfigManager::getWifiTimeout() {
  return preferences.getInt("wifi_timeout", 60);
}

void ConfigManager::setWifiTimeout(int seconds) {
  preferences.putInt("wifi_timeout", seconds);
}

std::string ConfigManager::getWebhook() {
  if (!preferences.isKey("webhook"))
    return "";
  return std::string(preferences.getString("webhook", "").c_str());
}

void ConfigManager::setWebhook(const std::string &url) {
  preferences.putString("webhook", url.c_str());
}

std::string ConfigManager::getSpoolmanUrl() {
  if (!preferences.isKey("spoolman"))
    return "";
  return std::string(preferences.getString("spoolman", "").c_str());
}

void ConfigManager::setSpoolmanUrl(const std::string &url) {
  preferences.putString("spoolman", url.c_str());
}

uint8_t ConfigManager::getNumTools() {
  if (!preferences.isKey("num_tools"))
    return getU1Host().empty() ? 1 : 4;
  return preferences.getUChar("num_tools", 1);
}

void ConfigManager::setNumTools(uint8_t tools) {
  if (tools < 1)
    tools = 1;
  if (tools > 16)
    tools = 16;
  preferences.putUChar("num_tools", tools);
}

void ConfigManager::clearNumTools() {
    preferences.remove("num_tools");
}

uint8_t ConfigManager::getPowerMode() {
  return preferences.getUChar("power_mode", 1);
}

void ConfigManager::setPowerMode(uint8_t mode) {
  preferences.putUChar("power_mode", mode);
}

uint16_t ConfigManager::getSleepTimeout() {
  return preferences.getUShort("sleep_timeout", 300);
}

void ConfigManager::setSleepTimeout(uint16_t seconds) {
  preferences.putUShort("sleep_timeout", seconds);
}

uint16_t ConfigManager::getDisplayTimeout() {
  return preferences.getUShort("display_timeout", 300);
}

void ConfigManager::setDisplayTimeout(uint16_t seconds) {
  preferences.putUShort("display_timeout", seconds);
}

std::string ConfigManager::getU1Host() {
  if (!preferences.isKey("u1_host"))
    return "";
  return std::string(preferences.getString("u1_host", "").c_str());
}

void ConfigManager::setU1Host(const std::string &host) {
  preferences.putString("u1_host", host.c_str());
}

std::string ConfigManager::getTagFormat() {
  if (!preferences.isKey("tag_format"))
    return "ask";
  return std::string(preferences.getString("tag_format", "ask").c_str());
}

void ConfigManager::setTagFormat(const std::string &format) {
  preferences.putString("tag_format", format.c_str());
}

#else
// SIMULATOR IMPLEMENTATION
#include <fstream>
#include <sstream>

std::map<std::string, std::string> ConfigManager::simConfig;

void ConfigManager::init() { loadSimConfig(); }

void ConfigManager::loadSimConfig() {
  // Populate with default test values
  simConfig["wifi_ssid"] = "Test_WiFi";
  simConfig["wifi_pass"] = "password123";
  simConfig["webhook"] = "http://192.168.1.100:8000/webhook";
  simConfig["spoolman"] = "http://192.168.1.100:7912";
  simConfig["num_tools"] = "4";
  simConfig["display_timeout"] = "0";
  simConfig["power_mode"] = "0"; // Always On for simulator
  simConfig["sleep_timeout"] = "5";
  simConfig["u1_host"] = "192.168.1.50";
  simConfig["tag_format"] = "ask";

  std::ifstream file("simulator/config.cfg");
  if (!file.is_open())
    return;

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    size_t delimiterPos = line.find('=');
    if (delimiterPos != std::string::npos) {
      std::string key = line.substr(0, delimiterPos);
      std::string value = line.substr(delimiterPos + 1);
      simConfig[key] = value;
    }
  }
}

std::string ConfigManager::getWifiSSID() {
  auto it = simConfig.find("wifi_ssid");
  return (it != simConfig.end()) ? it->second : "Simulator_WiFi";
}

void ConfigManager::setWifiSSID(const std::string &ssid) {}

std::string ConfigManager::getWifiPass() {
  auto it = simConfig.find("wifi_pass");
  return (it != simConfig.end()) ? it->second : "simulator_pass";
}

void ConfigManager::setWifiPass(const std::string &pass) {}

int ConfigManager::getWifiTimeout() {
  return 60;
}

void ConfigManager::setWifiTimeout(int seconds) {}

std::string ConfigManager::getWebhook() {
  auto it = simConfig.find("webhook");
  return (it != simConfig.end()) ? it->second : "";
}

void ConfigManager::setWebhook(const std::string &url) {}

std::string ConfigManager::getSpoolmanUrl() {
  auto it = simConfig.find("spoolman");
  return (it != simConfig.end()) ? it->second : "";
}

void ConfigManager::setSpoolmanUrl(const std::string &url) {}

uint8_t ConfigManager::getNumTools() {
  auto it = simConfig.find("num_tools");
  if (it != simConfig.end()) {
    try {
      return (uint8_t)std::stoi(it->second);
    } catch (...) {
    }
  }
  return getU1Host().empty() ? 1 : 4;
}

void ConfigManager::setNumTools(uint8_t tools) {}

uint8_t ConfigManager::getPowerMode() {
  auto it = simConfig.find("power_mode");
  if (it != simConfig.end()) {
    try {
      return (uint8_t)std::stoi(it->second);
    } catch (...) {}
  }
  return 0; // Default to Always On in simulator
}

void ConfigManager::setPowerMode(uint8_t mode) {}

uint16_t ConfigManager::getSleepTimeout() {
  auto it = simConfig.find("sleep_timeout");
  if (it != simConfig.end()) {
    try {
      return (uint16_t)std::stoi(it->second);
    } catch (...) {}
  }
  return 300;
}

void ConfigManager::setSleepTimeout(uint16_t seconds) {}

uint16_t ConfigManager::getDisplayTimeout() {
  auto it = simConfig.find("display_timeout");
  if (it != simConfig.end()) {
    try {
      return (uint16_t)std::stoi(it->second);
    } catch (...) {}
  }
  return  0; // Default to Always On 
}

void ConfigManager::setDisplayTimeout(uint16_t seconds) {}

std::string ConfigManager::getU1Host() {
  auto it = simConfig.find("u1_host");
  return (it != simConfig.end()) ? it->second : "";
}

void ConfigManager::setU1Host(const std::string &host) {}

std::string ConfigManager::getTagFormat() {
  auto it = simConfig.find("tag_format");
  if (it != simConfig.end())
    return it->second;
  return "ask";
}

void ConfigManager::setTagFormat(const std::string &format) {
  simConfig["tag_format"] = format;
}

#endif
