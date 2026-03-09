#include "NetworkManager.h"
#include "../config/ConfigManager.h"
#include "WebhookFormatter.h"
#include <ArduinoJson.h>
#ifndef USE_SDL2
#include <HTTPClient.h>
#include <WiFi.h>
#else
#include <curl/curl.h>
#endif

static bool ensureWiFi() {
#ifndef USE_SDL2
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  NetworkManager::connectWiFi();
  return (WiFi.status() == WL_CONNECTED);
#else
  return true; // Desktop always has "WiFi"
#endif
}

void NetworkManager::connectWiFi() {
#ifndef USE_SDL2
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
#endif
}

#ifdef USE_SDL2
static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}
#endif

bool NetworkManager::sendWebhookPayload(const std::string &spool_id,
                                        int toolhead_id) {
  std::string url = ConfigManager::getWebhook();

  if (url.empty()) {
#ifndef USE_SDL2
    Serial.println(
        "NetworkManager: No Webhook configured. Skipping Wi-Fi and payload.");
#else
    printf("NetworkManager: No Webhook configured.\n");
#endif
    return false;
  }

  // Ensure we're connected to Wi-Fi first
  if (!ensureWiFi()) {
    return false;
  }

  bool useGet = (url.find("{spool_id}") != std::string::npos);

#ifndef USE_SDL2
  HTTPClient http;
  int httpResponseCode = -1;
  bool success = false;

  if (useGet) {
    std::string formattedUrl =
        WebhookFormatter::formatUrl(url, spool_id, toolhead_id);
    http.begin(formattedUrl.c_str());
    httpResponseCode = http.GET();
  } else {
    JsonDocument doc;
    doc["spool_id"] = spool_id;
    doc["toolhead"] = toolhead_id;
    String jsonString;
    serializeJson(doc, jsonString);
    http.begin(url.c_str());
    http.addHeader("Content-Type", "application/json");
    httpResponseCode = http.POST(jsonString);
  }

  if (httpResponseCode >= 200 && httpResponseCode < 300)
    success = true;
  http.end();
  return success;
#else
  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  std::string formattedUrl = url;
  if (useGet) {
    formattedUrl = WebhookFormatter::formatUrl(url, spool_id, toolhead_id);
  }

  curl_easy_setopt(curl, CURLOPT_URL, formattedUrl.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  if (!useGet) {
    JsonDocument doc;
    doc["spool_id"] = spool_id;
    doc["toolhead"] = toolhead_id;
    std::string jsonStr;
    serializeJson(doc, jsonStr);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }

  CURLcode res = curl_easy_perform(curl);
  long response_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  curl_easy_cleanup(curl);

  return (res == CURLE_OK && response_code >= 200 && response_code < 300);
#endif
}

bool NetworkManager::fetchSpoolmanData(OpenSpoolData &data) {
  std::string baseUrl = ConfigManager::getSpoolmanUrl();
  if (baseUrl.empty() || data.spool_id.empty()) {
    return false;
  }

  if (!ensureWiFi()) {
    return false;
  }

  if (baseUrl.back() == '/')
    baseUrl.pop_back();
  std::string api_url = baseUrl + "/api/v1/spool/" + data.spool_id;

  std::string payload;
  long response_code = 0;

#ifndef USE_SDL2
  HTTPClient http;
  http.begin(api_url.c_str());
  response_code = http.GET();
  if (response_code == 200) {
    payload = http.getString().c_str();
  }
  http.end();
#else
  CURL *curl = curl_easy_init();
  if (curl) {
    printf("NetworkManager: curl_easy_perform to %s\n", api_url.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &payload);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
      printf("NetworkManager: curl_easy_perform failed: %s\n",
             curl_easy_strerror(res));
      response_code = 0;
    } else {
      printf("NetworkManager: curl_easy_perform success, code: %ld\n",
             response_code);
    }
  }
#endif

  if (response_code == 200) {
    printf("NetworkManager: Parsing Spoolman JSON...\n");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      if (doc.containsKey("filament") && doc["filament"].containsKey("name")) {
        data.filament_name = doc["filament"]["name"].as<const char *>();
        printf("NetworkManager:   Enriched filament name: %s\n",
               data.filament_name.c_str());
      }

      if (doc.containsKey("remaining_weight")) {
        float remaining = doc["remaining_weight"].as<float>();
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1fg", remaining);
        data.remaining_weight = buf;
      }

      float initial = doc["initial_weight"].as<float>();
      float spool = doc["spool_weight"].as<float>();
      float total = initial + spool;
      if (total > 0) {
        char buf[32];
        if (total >= 1000) {
          snprintf(buf, sizeof(buf), "%d,%03dg", (int)(total / 1000),
                   (int)total % 1000);
        } else {
          snprintf(buf, sizeof(buf), "%dg", (int)total);
        }
        data.total_weight = buf;
      }
      return true;
    }
  }
  return false;
}
