#include "NetworkManager.h"
#include "../config/ConfigManager.h"
#include "../data/OpenSpool.h"
#include "WebhookFormatter.h"
#include <ArduinoJson.h>
#ifndef USE_SDL2
#include <HTTPClient.h>
#include <WiFi.h>
#else
#include <curl/curl.h>
#endif

static bool ensureWiFi();

#define WIFI_IDLE_TIMEOUT_MS 60000UL
static unsigned long lastNetworkActivity = 0;

static bool ensureWiFi() {
#ifndef USE_SDL2
  if (WiFi.status() == WL_CONNECTED) {
    lastNetworkActivity = millis();
    return true;
  }
  NetworkManager::connectWiFi();
  if (WiFi.status() == WL_CONNECTED)
    lastNetworkActivity = millis();
  return (WiFi.status() == WL_CONNECTED);
#else
  return true;
#endif
}

void NetworkManager::tick() {
#ifndef USE_SDL2
  if (WiFi.status() == WL_CONNECTED &&
      millis() - lastNetworkActivity > WIFI_IDLE_TIMEOUT_MS) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
#endif
}

bool NetworkManager::isWiFiConnected() {
#ifndef USE_SDL2
  return WiFi.status() == WL_CONNECTED;
#else
  return true;
#endif
}

void NetworkManager::connectWiFi() {
#ifndef USE_SDL2
  std::string ssid = ConfigManager::getWifiSSID();
  std::string pass = ConfigManager::getWifiPass();

  if (ssid.empty()) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    retries++;
  }

#endif
}

#ifdef USE_SDL2
static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

static size_t HeaderCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  std::string header((char*)contents, realsize);
  
  // Case-insensitive check
  std::string h = header;
  for (auto & c: h) c = tolower(c);
  
  if (h.find("x-total-count:") == 0) {
      size_t pos = h.find(":");
      if (pos != std::string::npos) {
          int *total = (int*)userp;
          *total = atoi(header.substr(pos + 1).c_str());
      }
  }
  return realsize;
}
#endif

bool NetworkManager::sendWebhookPayload(const OpenSpoolData &data,
                                        int toolhead_id) {
  std::string spool_id = data.spool_id;
  std::string url = ConfigManager::getWebhook();
  std::string u1_host = ConfigManager::getU1Host();

  if (url.empty() && u1_host.empty()) {
    return false;
  }

  // Ensure we're connected to Wi-Fi first
  if (!ensureWiFi()) {
    return false;
  }

  if (!u1_host.empty()) {
    // Snapmaker U1 Direct API Integration (OpenSpool U1 Extended Format)
    JsonDocument doc;
    doc["channel"] = toolhead_id;
    JsonObject info = doc["info"].to<JsonObject>();

    info["VENDOR"] = data.brand;

    // Ensure MAIN_TYPE is uppercase as recommended by spec
    std::string mainType = data.type;
    for (char &c : mainType)
      c = toupper(c);
    info["MAIN_TYPE"] = mainType;

    info["SUB_TYPE"] = data.subtype;

    // Convert HEX to decimal int for RGB_1
    int rgb = 0;
    std::string hex = data.color_hex;
    if (!hex.empty()) {
      if (hex[0] == '#')
        hex = hex.substr(1);
      try {
        rgb = std::stoi(hex, nullptr, 16);
      } catch (...) {
        rgb = 0;
      }
    }
    info["RGB_1"] = rgb;
    info["ALPHA"] = data.alpha.empty() ? 255 : std::stoi(data.alpha);

    // Temperature parsing
    auto s2i = [](const std::string &s) {
      try {
        return std::stoi(s);
      } catch (...) {
        return 0;
      }
    };
    info["HOTEND_MIN_TEMP"] = s2i(data.min_temp);
    info["HOTEND_MAX_TEMP"] = s2i(data.max_temp);
    info["BED_TEMP"] = s2i(data.bed_min_temp);

    std::string payload;
    serializeJson(doc, payload);

    std::string u1_url = "http://" + u1_host;
    if (u1_host.find(':') == std::string::npos) {
      u1_url += ":7125";
    }
    u1_url += "/printer/filament_detect/set";

#ifndef USE_SDL2
    HTTPClient http;
    http.begin(u1_url.c_str());
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(payload.c_str());
    // Consume response to avoid 'flush() fail on fd' errors
    http.getString();
    http.end();
    return (code >= 200 && code < 300);
#else
    CURL *curl = curl_easy_init();
    if (!curl)
      return false;
    curl_easy_setopt(curl, CURLOPT_URL, u1_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);
    return (res == CURLE_OK && code >= 200 && code < 300);
#endif
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

  // Consume response to avoid 'flush() fail on fd' errors
  http.getString();
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
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
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
  // Always consume response body
  payload = http.getString().c_str();
  http.end();
#else
  CURL *curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &payload);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
      response_code = 0;
    }
  }
#endif

  if (response_code == 200) {
    return OpenSpoolParser::enrichFromSpoolman(payload, data);
  }
  return false;
}

bool NetworkManager::fetchSpoolmanList(int page, int limit, std::vector<SpoolmanItem>& items, int& total_count) {
  std::string baseUrl = ConfigManager::getSpoolmanUrl();
  if (baseUrl.empty()) {
    return false;
  }

  if (!ensureWiFi()) {
    return false;
  }

  if (baseUrl.back() == '/')
    baseUrl.pop_back();

  int offset = page * limit;
  std::string api_url = baseUrl + "/api/v1/spool?archived=false&finished=false&limit=" + 
                        std::to_string(limit) + "&offset=" + std::to_string(offset);

  std::string payload;
  long response_code = 0;

#ifndef USE_SDL2
  HTTPClient http;
  http.begin(api_url.c_str());
  const char *headerKeys[] = {"X-Total-Count"};
  http.collectHeaders(headerKeys, 1);
  
  response_code = http.GET();
  payload = http.getString().c_str();
  
  if (response_code == 200 && http.hasHeader("X-Total-Count")) {
      total_count = atoi(http.header("X-Total-Count").c_str());
  }
  http.end();
#else
  CURL *curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &payload);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &total_count);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
      response_code = 0;
    }
  }
#endif

  if (response_code == 200) {
    return OpenSpoolParser::parseSpoolmanList(payload, items, total_count);
  }
  return false;
}
