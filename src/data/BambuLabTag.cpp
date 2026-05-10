#include "BambuLabTag.h"
#include "../utils/HKDF.h"
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

// Bambu Lab RFID tag constants
static const size_t BLOCK_SIZE = 16;

// Block positions (Block number * 16)
static const size_t MATERIAL_ID_POS = 1 * BLOCK_SIZE + 8;
static const size_t FILAMENT_TYPE_POS = 2 * BLOCK_SIZE;
static const size_t DETAILED_TYPE_POS = 4 * BLOCK_SIZE;
static const size_t COLOR_RGBA_POS = 5 * BLOCK_SIZE;
static const size_t SPOOL_WEIGHT_POS = 5 * BLOCK_SIZE + 4;
static const size_t DIAMETER_POS = 5 * BLOCK_SIZE + 8;
static const size_t DRY_TEMP_POS = 6 * BLOCK_SIZE;
static const size_t DRY_TIME_POS = 6 * BLOCK_SIZE + 2;
static const size_t BED_TEMP_POS = 6 * BLOCK_SIZE + 6;
static const size_t HOTEND_MAX_POS = 6 * BLOCK_SIZE + 8;
static const size_t HOTEND_MIN_POS = 6 * BLOCK_SIZE + 10;
static const size_t TRAY_UID_POS = 9 * BLOCK_SIZE;
static const size_t PRODUCTION_DATE_POS = 12 * BLOCK_SIZE;

bool BambuLabTagParser::parse(const std::vector<uint8_t> &data,
                              const uint8_t *uid, OpenSpoolData &output) {
  if (data.size() < 128)
    return false; // Minimum sectors needed for basic info

  output.reset();
  output.protocol = "bambulab";
  output.brand = "Bambu Lab";

  // Extract hardware UID
  std::stringstream ss;
  ss << std::uppercase << std::hex;
  for (int i = 0; i < 4; i++) {
    ss << std::setw(2) << std::setfill('0') << (int)uid[i];
  }
  output.hardware_uid = ss.str();

  // Block 2: Filament Type (e.g., "PLA")
  output.type = extractString(&data[FILAMENT_TYPE_POS], 16);

  // Block 4: Detailed Type (e.g., "PLA Basic")
  std::string detailedType = extractString(&data[DETAILED_TYPE_POS], 16);
  if (!detailedType.empty() && detailedType != output.type) {
    output.subtype = detailedType;
  }

  // Block 5: Color (RGBA)
  uint8_t r = data[COLOR_RGBA_POS];
  uint8_t g = data[COLOR_RGBA_POS + 1];
  uint8_t b = data[COLOR_RGBA_POS + 2];
  uint8_t a = data[COLOR_RGBA_POS + 3];

  char colorBuf[10];
  snprintf(colorBuf, sizeof(colorBuf), "#%02X%02X%02X", r, g, b);
  output.color_hex = colorBuf;

  snprintf(colorBuf, sizeof(colorBuf), "%02X", a);
  output.alpha = colorBuf;

  // Block 5: Weight and Diameter
  uint16_t weight = extractUint16LE(&data[SPOOL_WEIGHT_POS]);
  output.total_weight = std::to_string(weight);

  float diameter = extractFloatLE(&data[DIAMETER_POS]);
  if (diameter > 0) {
    char diamBuf[16];
    snprintf(diamBuf, sizeof(diamBuf), "%.2f", diameter);
    output.diameter = diamBuf;
  }

  // Block 6: Temps
  output.dry_temp = std::to_string(extractUint16LE(&data[DRY_TEMP_POS]));
  output.dry_time = std::to_string(extractUint16LE(&data[DRY_TIME_POS]));
  output.bed_max_temp = std::to_string(extractUint16LE(&data[BED_TEMP_POS]));
  output.max_temp = std::to_string(extractUint16LE(&data[HOTEND_MAX_POS]));
  output.min_temp = std::to_string(extractUint16LE(&data[HOTEND_MIN_POS]));

  // Block 9: Tray UID (consistent across both tags of a spool)
  // We hex-ify the first 5 bytes for a compact but unique lot_nr (max 10 chars for UI)
  char lotBuf[11];
  const uint8_t* trayData = &data[TRAY_UID_POS];
  snprintf(lotBuf, sizeof(lotBuf), "%02X%02X%02X%02X%02X", 
           trayData[0], trayData[1], trayData[2], trayData[3],
           trayData[4]);
  output.lot_nr = lotBuf;

  // Set lot_nr fallback to hardware UID if still empty (though Tray UID should be there)
  if (output.lot_nr == "0000000000000000" && uid) {
    char uidBuf[13];
    snprintf(uidBuf, sizeof(uidBuf), "%02X%02X%02X%02X", uid[0], uid[1], uid[2],
             uid[3]);
    output.lot_nr = uidBuf;
  }

  return true;
}

bool BambuLabTagParser::deriveKeys(
    const uint8_t *uid, const uint8_t *salt,
    std::vector<std::vector<uint8_t>> &outputKeys) {
  outputKeys.clear();
  outputKeys.resize(16, std::vector<uint8_t>(6));

  uint8_t okm[6 * 16];
  const char *info = "RFID-A\0";

  if (!HKDF::deriveSHA256(salt, 16, uid, 4, (const uint8_t *)info, 7, okm,
                          sizeof(okm))) {
    return false;
  }

  for (int i = 0; i < 16; i++) {
    memcpy(outputKeys[i].data(), &okm[i * 6], 6);
  }

  return true;
}

float BambuLabTagParser::extractFloatLE(const uint8_t *data) {
  float val;
  memcpy(&val, data, 4);
  return val;
}

uint16_t BambuLabTagParser::extractUint16LE(const uint8_t *data) {
  return data[0] | (data[1] << 8);
}

uint32_t BambuLabTagParser::extractUint32LE(const uint8_t *data) {
  return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

std::string BambuLabTagParser::extractString(const uint8_t *data, size_t len) {
  std::string s;
  for (size_t i = 0; i < len; i++) {
    if (data[i] == 0)
      break;
    if (data[i] >= 32 && data[i] <= 126) {
      s += (char)data[i];
    }
  }
  // Trim trailing spaces
  size_t last = s.find_last_not_of(' ');
  if (std::string::npos != last) {
    s = s.substr(0, last + 1);
  }
  return s;
}
