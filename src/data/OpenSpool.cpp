#include "OpenSpool.h"

bool OpenSpoolParser::parseJson(const std::string &json, OpenSpoolData &data) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    return false;
  }

  if (doc["protocol"].is<std::string>())
    data.protocol = doc["protocol"].as<std::string>();
  if (doc["version"].is<std::string>())
    data.version = doc["version"].as<std::string>();
  if (doc["type"].is<std::string>())
    data.type = doc["type"].as<std::string>();
  if (doc["color_hex"].is<std::string>())
    data.color_hex = doc["color_hex"].as<std::string>();
  if (doc["brand"].is<std::string>())
    data.brand = doc["brand"].as<std::string>();
  if (doc["spool_id"].is<std::string>())
    data.spool_id = doc["spool_id"].as<std::string>();

  // Protocol check according to OpenSpool spec
  if (data.protocol != "openspool") {
    return false;
  }

  return true;
}

std::string OpenSpoolParser::toJson(const OpenSpoolData &data) {
  JsonDocument doc;

  doc["protocol"] = data.protocol;
  doc["version"] = data.version;
  doc["type"] = data.type;
  doc["color_hex"] = data.color_hex;
  doc["brand"] = data.brand;

  if (!data.spool_id.empty()) {
    doc["spool_id"] = data.spool_id;
  }

  std::string output;
  serializeJson(doc, output);
  return output;
}
