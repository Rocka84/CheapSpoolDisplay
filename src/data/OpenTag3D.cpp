#include "OpenTag3D.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

// Memory Map Offsets
#define OFFSET_VERSION 0x00
#define OFFSET_MATERIAL 0x02
#define OFFSET_MODIFIER 0x07
#define OFFSET_BRAND 0x1B
#define OFFSET_COLOR_NAME 0x2B
#define OFFSET_COLOR_HEX 0x4B
#define OFFSET_DIAMETER 0x5C
#define OFFSET_WEIGHT 0x5E
#define OFFSET_TEMP_NOZZLE 0x60
#define OFFSET_TEMP_BED 0x61
#define OFFSET_URL 0x70

bool OpenTag3DParser::parseBinary(const std::vector<uint8_t>& payload, OpenSpoolData& data) {
    if (payload.size() < 0x60) { // Core minimum
        return false;
    }

    data.reset();
    data.protocol = "opentag3d";

    // Version
    uint16_t version = readUint16BE(&payload[OFFSET_VERSION]);
    char verStr[10];
    snprintf(verStr, sizeof(verStr), "%d.%03d", version / 1000, version % 1000);
    data.version = verStr;

    // Material & Brand
    data.type = trimString((const char*)&payload[OFFSET_MATERIAL], 5);
    std::string modifier = trimString((const char*)&payload[OFFSET_MODIFIER], 5);
    if (!modifier.empty()) {
        data.subtype = modifier;
    }
    
    data.brand = trimString((const char*)&payload[OFFSET_BRAND], 16);
    data.filament_name = trimString((const char*)&payload[OFFSET_COLOR_NAME], 32);
    
    // Color
    data.color_hex = "#";
    char hex[3];
    // RGB
    for (int i = 0; i < 3; i++) {
        snprintf(hex, sizeof(hex), "%02X", payload[OFFSET_COLOR_HEX + i]);
        data.color_hex += hex;
    }
    // Alpha
    snprintf(hex, sizeof(hex), "%02X", payload[OFFSET_COLOR_HEX + 3]);
    data.alpha = hex;

    // Temps (stored as C/5)
    int nozzle = payload[OFFSET_TEMP_NOZZLE] * 5;
    int bed = payload[OFFSET_TEMP_BED] * 5;
    
    if (nozzle > 0) {
        data.min_temp = std::to_string(nozzle);
        data.max_temp = std::to_string(nozzle);
    }
    
    if (bed > 0) {
        data.bed_min_temp = std::to_string(bed);
        data.bed_max_temp = std::to_string(bed);
    }

    // Weight & Diameter
    uint16_t diameterUm = readUint16BE(&payload[OFFSET_DIAMETER]);
    uint16_t weightG = readUint16BE(&payload[OFFSET_WEIGHT]);
    
    if (diameterUm > 0) {
        char diaStr[16];
        snprintf(diaStr, sizeof(diaStr), "%.3f", (float)diameterUm / 1000.0f);
        // Remove trailing zeros for cleanliness
        data.lot_nr = "Parsed from Binary"; // Just as a marker for now
    }
    
    if (weightG > 0) {
        data.total_weight = std::to_string(weightG);
    }

    // Extended Fields (URL)
    // The standard specifies 32 bytes at 0x70
    if (payload.size() >= OFFSET_URL + 32) {
        std::string url = trimString((const char*)&payload[OFFSET_URL], 32);
        if (!url.empty()) {
            // Pattern: /spool/show/(\d+)
            const char* pattern = "/spool/show/";
            size_t pos = url.find(pattern);
            if (pos != std::string::npos) {
                std::string idPart = url.substr(pos + strlen(pattern));
                // Extract only digits
                std::string id = "";
                for (char c : idPart) {
                    if (isdigit(c)) id += c;
                    else break;
                }
                if (!id.empty()) {
                    data.spool_id = id;
                }
            }
        }
    }

    return true;
}

std::string OpenTag3DParser::trimString(const char* data, size_t maxLen) {
    std::string s = "";
    for (size_t i = 0; i < maxLen; i++) {
        if (data[i] == 0) break;
        // Basic UTF-8 or ASCII printable
        if ((unsigned char)data[i] >= 32) {
            s += data[i];
        }
    }
    // Trim trailing spaces
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    
    return s;
}

uint16_t OpenTag3DParser::readUint16BE(const uint8_t* data) {
    return (uint16_t)(data[0] << 8) | data[1];
}
