#include "../../src/config/ConfigManager.h"
#include <Arduino.h>
#include <unity.h>

// Define unity functions setup and teardown
void setUp(void) {
  // initialize ConfigManager
  ConfigManager::init();
}

void tearDown(void) {
  // clean stuff up here
}

void test_nvs_wifi_credentials() {
  // Write fake credentials
  std::string fake_ssid = "TestWiFiNetwork";
  std::string fake_pass = "TestPassword123!";

  ConfigManager::setWifiSSID(fake_ssid);
  ConfigManager::setWifiPass(fake_pass);

  // Read them back
  std::string read_ssid = ConfigManager::getWifiSSID();
  std::string read_pass = ConfigManager::getWifiPass();

  TEST_ASSERT_EQUAL_STRING(fake_ssid.c_str(), read_ssid.c_str());
  TEST_ASSERT_EQUAL_STRING(fake_pass.c_str(), read_pass.c_str());
}

void test_nvs_webhook_url() {
  std::string fake_url = "http://192.168.1.10:8000/webhook";

  ConfigManager::setWebhookUrl(fake_url);
  std::string read_url = ConfigManager::getWebhookUrl();

  TEST_ASSERT_EQUAL_STRING(fake_url.c_str(), read_url.c_str());
}

void test_nvs_format_erases_data() {
  // Assume previous tests wrote data. Now erase it.
  ConfigManager::setWifiSSID("");
  ConfigManager::setWifiPass("");
  ConfigManager::setWebhookUrl("");

  TEST_ASSERT_EQUAL_STRING("", ConfigManager::getWifiSSID().c_str());
  TEST_ASSERT_EQUAL_STRING("", ConfigManager::getWifiPass().c_str());
  TEST_ASSERT_EQUAL_STRING("", ConfigManager::getWebhookUrl().c_str());
}

void setup() {
  // Wait for >2 secs
  delay(2000);

  UNITY_BEGIN();
  RUN_TEST(test_nvs_wifi_credentials);
  RUN_TEST(test_nvs_webhook_url);
  RUN_TEST(test_nvs_format_erases_data);
  UNITY_END();
}

void loop() { delay(100); }
