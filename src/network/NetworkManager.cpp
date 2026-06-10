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
  int maxRetries = ConfigManager::getWifiTimeout() * 2; // 500ms delay per tick = 2 ticks per second
  while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
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
    // Snapmaker U1 Direct API Integration
    JsonDocument doc;
    doc["channel"] = toolhead_id;
    JsonObject info = doc["info"].to<JsonObject>();

    // --- Attempt CARD_UID path first ---
    // Normalize the hardware_uid: strip 0x/0X, colons, hyphens, spaces, quotes,
    // whitespace; uppercase. Then convert each byte pair to a decimal int.
    std::string rawUid = data.hardware_uid;
    Serial.printf("[U1] Raw UID from tag: '%s'\n", rawUid.c_str());

    std::string normUid;
    normUid.reserve(rawUid.size());
    for (char c : rawUid) {
      if (c == ' ' || c == ':' || c == '-' || c == '\t' || c == '\n' || c == '\r' || c == '"')
        continue;
      normUid += (char)toupper((unsigned char)c);
    }
    if (normUid.size() >= 2 && normUid[0] == '0' && normUid[1] == 'X')
      normUid = normUid.substr(2);

    Serial.printf("[U1] Normalized UID: '%s'\n", normUid.c_str());

    // Validate: non-empty, even length, only hex chars
    bool uidValid = !normUid.empty() && (normUid.size() % 2 == 0);
    if (uidValid) {
      for (char c : normUid) {
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
          uidValid = false;
          break;
        }
      }
    }

    if (uidValid) {
      // Build CARD_UID as array of decimal byte values
      JsonArray cardUid = info["CARD_UID"].to<JsonArray>();
      std::string byteLog = "[";
      for (size_t i = 0; i < normUid.size(); i += 2) {
        int byteVal = std::stoi(normUid.substr(i, 2), nullptr, 16);
        cardUid.add(byteVal);
        byteLog += std::to_string(byteVal);
        if (i + 2 < normUid.size()) byteLog += ",";
      }
      byteLog += "]";
      Serial.printf("[U1] Sending CARD_UID: %s\n", byteLog.c_str());
    } else {
      // Fallback: no valid UID — send full vendor/material/color data
      if (!rawUid.empty())
        Serial.println("[U1] UID invalid, falling back to vendor/material payload");

      info["VENDOR"] = data.brand;

      std::string mainType = data.type;
      for (char &c : mainType) c = toupper(c);
      info["MAIN_TYPE"] = mainType;

      info["SUB_TYPE"] = data.subtype;

      int rgb = 0;
      std::string hex = data.color_hex;
      if (!hex.empty()) {
        if (hex[0] == '#') hex = hex.substr(1);
        try { rgb = std::stoi(hex, nullptr, 16); } catch (...) { rgb = 0; }
      }
      info["RGB_1"] = rgb;
      info["ALPHA"] = data.alpha.empty() ? 255 : std::stoi(data.alpha, nullptr, 16);

      auto s2i = [](const std::string &s) {
        try { return std::stoi(s); } catch (...) { return 0; }
      };
      info["HOTEND_MIN_TEMP"] = s2i(data.min_temp);
      info["HOTEND_MAX_TEMP"] = s2i(data.max_temp);
      info["BED_TEMP"] = s2i(data.bed_min_temp);

      int s_id = 0;
      try { s_id = std::stoi(data.spool_id); } catch (...) { s_id = 0; }
      info["SPOOL_ID"] = s_id;
    }

    std::string payload;
    serializeJson(doc, payload);
    Serial.printf("[U1] Payload: %s\n", payload.c_str());

    std::string u1_url = "http://" + u1_host;
    if (u1_host.find(':') == std::string::npos)
      u1_url += ":7125";
    u1_url += "/printer/filament_detect/set";

#ifndef USE_SDL2
    HTTPClient http;
    http.begin(u1_url.c_str());
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(payload.c_str());
    std::string response = http.getString().c_str();
    http.end();
    Serial.printf("[U1] Response %d: %s\n", code, response.c_str());
    return (code >= 200 && code < 300);
#else
    CURL *curl = curl_easy_init();
    if (!curl)
      return false;
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, u1_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    Serial.printf("[U1] Response %ld: %s\n", code, response.c_str());
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
  if (baseUrl.empty()) {
    return false;
  }

  if (data.spool_id.empty()) {
    if (data.hardware_uid.empty() && data.lot_nr.empty()) return false;
    // Fallback: search by card_uid or lot_nr
    if (fetchSpoolmanByExternalId(data)) {
        // Continue to fetch full details if we found an ID
    } else {
        return false;
    }
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

// Normalize a UID string: uppercase, strip spaces/colons/hyphens/tabs/newlines/quotes, remove 0x prefix.
static std::string normalizeUid(const std::string &uid) {
  std::string out;
  out.reserve(uid.size());
  for (char c : uid) {
    if (c == ' ' || c == ':' || c == '-' || c == '\t' || c == '\n' || c == '\r' || c == '"')
      continue;
    out += (char)toupper((unsigned char)c);
  }
  if (out.size() >= 2 && out[0] == '0' && out[1] == 'X')
    out = out.substr(2);
  return out;
}

bool NetworkManager::fetchSpoolmanByExternalId(OpenSpoolData &data) {
  std::string baseUrl = ConfigManager::getSpoolmanUrl();
  if (baseUrl.empty() || (data.hardware_uid.empty() && data.lot_nr.empty())) {
    return false;
  }

  if (!ensureWiFi()) {
    return false;
  }

  if (baseUrl.back() == '/')
    baseUrl.pop_back();

  // Fetch all spools (including archived) so both card_uids and lot_nr can be checked in one pass.
  std::string api_url = baseUrl + "/api/v1/spool?allow_archived=true";

  std::string payload;
  long response_code = 0;

#ifndef USE_SDL2
  HTTPClient http;
  http.begin(api_url.c_str());
  response_code = http.GET();
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
  }
#endif

  if (response_code != 200) return false;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error || !doc.is<JsonArray>()) return false;

  JsonArray spools = doc.as<JsonArray>();
  std::string normHwUid = normalizeUid(data.hardware_uid);
  int lotNrMatchIdx = -1;

  for (int i = 0; i < (int)spools.size(); i++) {
    JsonObject spool = spools[i];

    // 1. card_uids check (priority)
    if (!normHwUid.empty()) {
      JsonVariant cardUidsVar = spool["extra"]["card_uids"];
      if (!cardUidsVar.isNull()) {
        std::string cardUidsStr = cardUidsVar.as<std::string>();
        // Spoolman encodes extra values as JSON strings — strip surrounding quotes.
        if (cardUidsStr.size() >= 2 && cardUidsStr.front() == '"' && cardUidsStr.back() == '"')
          cardUidsStr = cardUidsStr.substr(1, cardUidsStr.size() - 2);

        // Split comma-separated list and compare each entry.
        size_t start = 0;
        while (start <= cardUidsStr.size()) {
          size_t end = cardUidsStr.find(',', start);
          if (end == std::string::npos) end = cardUidsStr.size();
          if (normalizeUid(cardUidsStr.substr(start, end - start)) == normHwUid) {
            if (spool["id"].is<int>())
              data.spool_id = std::to_string(spool["id"].as<int>());
            else
              data.spool_id = spool["id"].as<std::string>();
            Serial.println("[Spoolman] Matched by card_uid");
            return true;
          }
          start = end + 1;
        }
      }
    }

    // 2. Remember first lot_nr match as fallback.
    if (lotNrMatchIdx < 0 && !data.lot_nr.empty()) {
      JsonVariant lotVar = spool["lot_nr"];
      if (!lotVar.isNull() && lotVar.as<std::string>() == data.lot_nr)
        lotNrMatchIdx = i;
    }
  }

  // Fallback: lot_nr
  if (lotNrMatchIdx >= 0) {
    JsonObject spool = spools[lotNrMatchIdx];
    if (spool["id"].is<int>())
      data.spool_id = std::to_string(spool["id"].as<int>());
    else
      data.spool_id = spool["id"].as<std::string>();
    return true;
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
