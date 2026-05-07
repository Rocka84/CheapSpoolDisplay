#include "WriteGuard.h"
#include <ArduinoJson.h>

size_t WriteGuard::estimateSize(const OpenSpoolData& data, const std::string& format) {
    if (format == "opentag3d") return 200;
    if (format == "openprinttag") return 300; // Rough average
    
    // JSON estimation
    StaticJsonDocument<1024> doc;
    doc["protocol"] = data.protocol;
    doc["version"] = data.version;
    if (!data.type.empty()) doc["type"] = data.type;
    if (!data.brand.empty()) doc["brand"] = data.brand;
    if (!data.color_hex.empty()) doc["color_hex"] = data.color_hex;
    if (!data.spool_id.empty()) doc["spool_id"] = data.spool_id;
    if (!data.notes.empty()) doc["notes"] = data.notes;
    // ... add most fields
    return measureJson(doc);
}

void WriteGuard::applySafeguards(OpenSpoolData& data, size_t capacity) {
    // If we are on a tight tag (NTAG215 ~500 bytes)
    if (capacity <= 512) {
        if (data.notes.length() > 64) {
            data.notes = data.notes.substr(0, 61) + "...";
        }
        if (data.tags.length() > 32) {
            data.tags = data.tags.substr(0, 29) + "...";
        }
    }
}

size_t WriteGuard::getTagCapacity(const std::string& type) {
    if (type == "NTAG213") return 144;
    if (type == "NTAG215") return 504;
    if (type == "NTAG216") return 888;
    return 500; // Default safe estimate
}
