#include "OpenSpool.h"

bool OpenSpoolParser::parseJson(const std::string &json, OpenSpoolData &data) {
  data.reset();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    return false;
  }

  if (!doc["protocol"].is<std::string>() ||
      doc["protocol"].as<std::string>() != "openspool") {
    return false;
  }

  data.protocol = doc["protocol"].as<std::string>();
  if (doc["version"].is<std::string>()) {
    data.version = doc["version"].as<std::string>();
  } else if (doc["version"].is<int>()) {
    data.version = std::to_string(doc["version"].as<int>());
  }
  if (doc["type"].is<std::string>())
    data.type = doc["type"].as<std::string>();
  if (doc["color_hex"].is<std::string>()) {
    std::string hex = doc["color_hex"].as<std::string>();
    if (!hex.empty() && hex[0] != '#') {
      hex = "#" + hex;
    }
    data.color_hex = hex;
  }

  if (doc["brand"].is<std::string>())
    data.brand = doc["brand"].as<std::string>();

  if (doc["spool_id"].is<std::string>()) {
    data.spool_id = doc["spool_id"].as<std::string>();
  } else if (doc["spool_id"].is<int>()) {
    data.spool_id = std::to_string(doc["spool_id"].as<int>());
  }

  if (doc["min_temp"].is<std::string>())
    data.min_temp = doc["min_temp"].as<std::string>();
  if (doc["max_temp"].is<std::string>())
    data.max_temp = doc["max_temp"].as<std::string>();
  if (doc["bed_min_temp"].is<std::string>())
    data.bed_min_temp = doc["bed_min_temp"].as<std::string>();
  if (doc["bed_max_temp"].is<std::string>())
    data.bed_max_temp = doc["bed_max_temp"].as<std::string>();
  if (doc["lot_nr"].is<std::string>())
    data.lot_nr = doc["lot_nr"].as<std::string>();
  if (doc["subtype"].is<std::string>())
    data.subtype = doc["subtype"].as<std::string>();
  if (doc["alpha"].is<std::string>())
    data.alpha = doc["alpha"].as<std::string>();

  // Protocol check according to OpenSpool spec
  if (data.protocol != "openspool") {
    return false;
  }

  return true;
}

bool OpenSpoolParser::enrichFromSpoolman(const std::string &json,
                                         OpenSpoolData &data) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    return false;
  }

  if (doc["filament"]["name"].is<std::string>()) {
    data.filament_name = doc["filament"]["name"].as<std::string>();
  }

  // Bidirectional sync: If tag data is missing manufacturer info, fill from Spoolman
  if (data.brand.empty() && doc["filament"]["vendor"]["name"].is<std::string>()) {
    data.brand = doc["filament"]["vendor"]["name"].as<std::string>();
  }
  if (data.type.empty() && doc["filament"]["material"].is<std::string>()) {
    data.type = doc["filament"]["material"].as<std::string>();
  }
  if (data.color_hex.empty() && doc["filament"]["color_hex"].is<std::string>()) {
    std::string hex = doc["filament"]["color_hex"].as<std::string>();
    if (!hex.empty() && hex[0] != '#') hex = "#" + hex;
    data.color_hex = hex;
  }
  if (data.max_temp.empty() && doc["filament"]["settings_extruder_temp"].is<int>()) {
    data.max_temp = std::to_string(doc["filament"]["settings_extruder_temp"].as<int>());
  }
  if (data.bed_max_temp.empty() && doc["filament"]["settings_bed_temp"].is<int>()) {
    data.bed_max_temp = std::to_string(doc["filament"]["settings_bed_temp"].as<int>());
  }

  if (doc["remaining_weight"].is<float>()) {
    float remaining = doc["remaining_weight"].as<float>();
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1fg", remaining);
    data.remaining_weight = buf;
  }

  if (doc["initial_weight"].is<float>() && doc["spool_weight"].is<float>()) {
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
  }

  // Finalize temperature ranges: if only one side is present, mirror it
  if (data.min_temp.empty() && !data.max_temp.empty()) data.min_temp = data.max_temp;
  if (data.max_temp.empty() && !data.min_temp.empty()) data.max_temp = data.min_temp;
  if (data.bed_min_temp.empty() && !data.bed_max_temp.empty()) data.bed_min_temp = data.bed_max_temp;
  if (data.bed_max_temp.empty() && !data.bed_min_temp.empty()) data.bed_max_temp = data.bed_min_temp;

  return true;
}

bool OpenSpoolParser::parseSpoolmanList(const std::string &json, std::vector<SpoolmanItem> &items, int &total_count) {
  items.clear();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    return false;
  }

  JsonArray list;
  if (doc.is<JsonArray>()) {
    list = doc.as<JsonArray>();
    if (total_count <= 0) {
        total_count = list.size(); 
    }
  } else if (doc["items"].is<JsonArray>()) {
    list = doc["items"].as<JsonArray>();
    if (doc["total_count"].is<int>()) {
        total_count = doc["total_count"].as<int>();
    } else if (total_count <= 0) {
        total_count = list.size();
    }
  } else {
    return false;
  }

  for (JsonObject obj : list) {
    SpoolmanItem item;
    if (obj["id"].is<int>()) {
        item.id = std::to_string(obj["id"].as<int>());
    } else if (obj["id"].is<std::string>()) {
        item.id = obj["id"].as<std::string>();
    }
    
    JsonObject filament = obj["filament"];
    if (filament["vendor"]["name"].is<std::string>()) {
        item.brand = filament["vendor"]["name"].as<std::string>();
    }
    if (filament["material"].is<std::string>()) {
        item.type = filament["material"].as<std::string>();
    }
    if (filament["name"].is<std::string>()) {
        item.name = filament["name"].as<std::string>();
    }
    if (filament["color_hex"].is<std::string>()) {
        item.color_hex = filament["color_hex"].as<std::string>();
    }
    if (obj["remaining_weight"].is<float>()) {
        item.remaining_weight = obj["remaining_weight"].as<float>();
    }
    items.push_back(item);
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
  if (!data.min_temp.empty())
    doc["min_temp"] = data.min_temp;
  if (!data.max_temp.empty())
    doc["max_temp"] = data.max_temp;
  if (!data.bed_min_temp.empty())
    doc["bed_min_temp"] = data.bed_min_temp;
  if (!data.bed_max_temp.empty())
    doc["bed_max_temp"] = data.bed_max_temp;
  if (!data.lot_nr.empty())
    doc["lot_nr"] = data.lot_nr;
  if (!data.subtype.empty())
    doc["subtype"] = data.subtype;
  if (!data.alpha.empty())
    doc["alpha"] = data.alpha;

  std::string output;
  serializeJson(doc, output);
  return output;
}
