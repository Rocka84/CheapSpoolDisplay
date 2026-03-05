#include "NetworkManager.h"
#include "../config/ConfigManager.h"
#include "WebhookFormatter.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

void NetworkManager::connectWiFi() {
  std::string ssid = ConfigManager::getWifiSSID();
  std::string pass = ConfigManager::getWifiPass();

  if (ssid.empty()) {
    Serial.println("NetworkManager: No SSID configured. Skipping Wi-Fi.");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    return; // Already connected
  }

  Serial.printf("NetworkManager: Connecting to Wi-Fi SSID '%s'...\n",
                ssid.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());

  // Wait up to 10 seconds for connection
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nNetworkManager: Wi-Fi Connected! IP: %s\n",
                  WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nNetworkManager: Failed to connect to Wi-Fi.");
  }
}

bool NetworkManager::sendWebhookPayload(const std::string &spool_id,
                                        int toolhead_id) {
  std::string url = ConfigManager::getWebhook();

  if (url.empty()) {
    Serial.println(
        "NetworkManager: No Webhook configured. Skipping Wi-Fi and payload.");
    return false;
  }

  // Ensure we're connected to Wi-Fi first
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("NetworkManager: Aborting Webhook, no Wi-Fi.");
      return false;
    }
  }

  bool useGet = (url.find("{spool_id}") != std::string::npos);

  HTTPClient http;
  int httpResponseCode = -1;
  bool success = false;

  if (useGet) {
    // Interpolate URL for GET requests
    std::string formattedUrl =
        WebhookFormatter::formatUrl(url, spool_id, toolhead_id);
    String finalUrl = String(formattedUrl.c_str());

    Serial.printf("NetworkManager: Sending GET to %s\n", finalUrl.c_str());
    http.begin(finalUrl);
    httpResponseCode = http.GET();

  } else {
    // Standard JSON POST
    JsonDocument doc;
    doc["spool_id"] = spool_id;
    doc["toolhead"] = toolhead_id;

    String jsonString;
    serializeJson(doc, jsonString);

    Serial.printf("NetworkManager: Sending POST to %s\n", url.c_str());
    http.begin(url.c_str());
    http.addHeader("Content-Type", "application/json");
    httpResponseCode = http.POST(jsonString);
  }

  if (httpResponseCode > 0) {
    Serial.printf("NetworkManager: HTTP Response code: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println(response);
    if (httpResponseCode >= 200 && httpResponseCode < 300) {
      success = true;
    }
  } else {
    Serial.printf("NetworkManager: Error code: %d\n", httpResponseCode);
  }

  http.end();
  return success;
}
