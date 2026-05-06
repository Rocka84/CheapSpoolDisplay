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
        data.diameter = diaStr;
    }
    data.lot_nr = ""; // Not supported in binary format
    
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

std::vector<uint8_t> OpenTag3DParser::generateBinary(const OpenSpoolData& data) {
    std::vector<uint8_t> payload(200, 0); // Standard tag size used in mock

    // Version (Fixed 1.000)
    writeUint16BE(&payload[OFFSET_VERSION], 1000);

    // Material
    std::string mat = data.type;
    if (mat.length() > 5) mat = mat.substr(0, 5);
    memcpy(&payload[OFFSET_MATERIAL], mat.c_str(), mat.length());

    // Modifier (Subtype)
    std::string modifier = data.subtype;
    if (modifier.length() > 5) modifier = modifier.substr(0, 5);
    memcpy(&payload[OFFSET_MODIFIER], modifier.c_str(), modifier.length());

    // Brand
    std::string brand = data.brand;
    if (brand.length() > 16) brand = brand.substr(0, 16);
    memcpy(&payload[OFFSET_BRAND], brand.c_str(), brand.length());

    // Color Name (mapped from filament_name)
    std::string colorName = data.filament_name;
    if (colorName.length() > 32) colorName = colorName.substr(0, 32);
    memcpy(&payload[OFFSET_COLOR_NAME], colorName.c_str(), colorName.length());

    // Color Hex (#RRGGBB)
    if (data.color_hex.length() >= 7 && data.color_hex[0] == '#') {
        for (int i = 0; i < 3; i++) {
            std::string byteStr = data.color_hex.substr(1 + i * 2, 2);
            payload[OFFSET_COLOR_HEX + i] = (uint8_t)strtol(byteStr.c_str(), nullptr, 16);
        }
    }
    // Alpha
    if (!data.alpha.empty()) {
        payload[OFFSET_COLOR_HEX + 3] = (uint8_t)strtol(data.alpha.c_str(), nullptr, 16);
    } else {
        payload[OFFSET_COLOR_HEX + 3] = 0xFF; // Solid
    }

    // Temperatures (stored as C/5)
    if (!data.max_temp.empty()) {
        payload[OFFSET_TEMP_NOZZLE] = (uint8_t)(atoi(data.max_temp.c_str()) / 5);
    }
    if (!data.bed_max_temp.empty()) {
        payload[OFFSET_TEMP_BED] = (uint8_t)(atoi(data.bed_max_temp.c_str()) / 5);
    }

    // Diameter (µm)
    float dia = 1.75f; // Default
    if (!data.diameter.empty()) {
        dia = atof(data.diameter.c_str());
    } else if (data.type.find("1.75") != std::string::npos) {
        dia = 1.75f;
    } else if (data.type.find("2.85") != std::string::npos) {
        dia = 2.85f;
    }
    writeUint16BE(&payload[OFFSET_DIAMETER], (uint16_t)(dia * 1000));

    // Weight
    if (!data.total_weight.empty()) {
        writeUint16BE(&payload[OFFSET_WEIGHT], (uint16_t)atoi(data.total_weight.c_str()));
    }

    // Online Data URL (Spoolman)
    if (!data.spool_id.empty()) {
        std::string url = "spoolman.local/spool/show/" + data.spool_id;
        if (url.length() > 32) url = url.substr(0, 32);
        memcpy(&payload[OFFSET_URL], url.c_str(), url.length());
    }

    return payload;
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

void OpenTag3DParser::writeUint16BE(uint8_t* data, uint16_t value) {
    data[0] = (uint8_t)((value >> 8) & 0xFF);
    data[1] = (uint8_t)(value & 0xFF);
}
