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
    return 1;
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
  if (!preferences.isKey("timeout"))
    return 60;
  return preferences.getUShort("timeout", 60);
}

void ConfigManager::setScreenTimeout(uint16_t seconds) {
  if (seconds < 10)
    seconds = 10;
  if (seconds > 3600)
    seconds = 3600;
  preferences.putUShort("timeout", seconds);
}

std::string ConfigManager::getU1Host() {
  if (!preferences.isKey("u1_host"))
    return "";
  return std::string(preferences.getString("u1_host", "").c_str());
}

void ConfigManager::setU1Host(const std::string &host) {
  preferences.putString("u1_host", host.c_str());
}

#else
// SIMULATOR IMPLEMENTATION
#include <fstream>
#include <sstream>

std::map<std::string, std::string> ConfigManager::simConfig;

void ConfigManager::init() { loadSimConfig(); }

void ConfigManager::loadSimConfig() {
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
  return 4;
}

void ConfigManager::setNumTools(uint8_t tools) {}

uint16_t ConfigManager::getScreenTimeout() {
  auto it = simConfig.find("timeout");
  if (it != simConfig.end()) {
    try {
      return (uint16_t)std::stoi(it->second);
    } catch (...) {
    }
  }
  return 60;
}

void ConfigManager::setScreenTimeout(uint16_t seconds) {}

std::string ConfigManager::getU1Host() {
  auto it = simConfig.find("u1_host");
  return (it != simConfig.end()) ? it->second : "";
}

void ConfigManager::setU1Host(const std::string &host) {}

#endif
