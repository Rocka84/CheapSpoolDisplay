#include "SnapmakerTag.h"
#ifdef ESP32
#include <mbedtls/md.h>
#endif
#include <cstring>
#include <cstdio>

// Salts from Snapmaker firmware
const char* SALT_A = "Snapmaker_qwertyuiop[,.;]";
const char* SALT_B = "Snapmaker_qwertyuiop[,.;]_1q2w3e";

void SnapmakerTagParser::deriveKey(const uint8_t* uid, uint8_t sectorIndex, char keyType, uint8_t* outputKey) {
#ifdef ESP32
    const char* salt = (keyType == 'a' || keyType == 'A') ? SALT_A : SALT_B;
    
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    
    // Step 1: Extract (PRK)
    uint8_t prk[32];
    mbedtls_md_hmac_starts(&ctx, (const uint8_t*)salt, strlen(salt));
    mbedtls_md_hmac_update(&ctx, uid, 4);
    mbedtls_md_hmac_finish(&ctx, prk);
    
    // Step 2: Expand
    char info[32];
    snprintf(info, sizeof(info), "key_%c_%d", (keyType == 'a' || keyType == 'A') ? 'a' : 'b', sectorIndex);
    
    uint8_t expanded[32];
    mbedtls_md_hmac_starts(&ctx, prk, 32);
    mbedtls_md_hmac_update(&ctx, (const uint8_t*)info, strlen(info));
    uint8_t counter = 1;
    mbedtls_md_hmac_update(&ctx, &counter, 1);
    mbedtls_md_hmac_finish(&ctx, expanded);
    
    memcpy(outputKey, expanded, 6);
    
    mbedtls_md_free(&ctx);
#else
    // Dummy implementation for simulator/native
    memset(outputKey, 0, 6);
#endif
}

bool SnapmakerTagParser::parse(const std::vector<uint8_t>& data, const uint8_t* uid, OpenSpoolData& output) {
    if (data.size() < 640) return false; // Need at least sectors 0-9

    output.reset();
    output.brand = "Snapmaker";
    output.protocol = "snapmaker";

    // Sector 0
    // Vendor (Block 1)
    char vendor[17];
    memcpy(vendor, &data[1 * 16], 16);
    vendor[16] = '\0';
    // Manufacturer (Block 2)
    char manufacturer[17];
    memcpy(manufacturer, &data[2 * 16], 16);
    manufacturer[16] = '\0';
    
    if (strlen(vendor) > 0) output.brand = vendor;

    // Sector 1
    // Version (Block 0, bytes 0-1)
    uint16_t version = data[1 * 64 + 0] | (data[1 * 64 + 1] << 8);
    output.version = std::to_string(version);

    // Main Type (Block 0, bytes 2-3)
    uint16_t mainType = data[1 * 64 + 2] | (data[1 * 64 + 3] << 8);
    output.type = mapMainType(mainType);

    // Sub Type (Block 0, bytes 4-5)
    uint16_t subType = data[1 * 64 + 4] | (data[1 * 64 + 5] << 8);
    output.subtype = mapSubType(subType);

    // Alpha (Block 0, byte 9) - Snapmaker stores 0xFF - Alpha
    uint8_t alpha = 0xFF - data[1 * 64 + 9];
    char alphaBuf[4];
    snprintf(alphaBuf, sizeof(alphaBuf), "%02X", alpha);
    output.alpha = alphaBuf;

    // RGB 1 (Block 1, bytes 0-2)
    uint32_t rgb = (data[1 * 64 + 16 + 0] << 16) | (data[1 * 64 + 16 + 1] << 8) | data[1 * 64 + 16 + 2];
    char colorBuf[8];
    snprintf(colorBuf, sizeof(colorBuf), "#%06X", rgb);
    output.color_hex = colorBuf;

    // SKU (Block 2, bytes 0-3)
    uint32_t sku = data[1 * 64 + 32 + 0] | (data[1 * 64 + 32 + 1] << 8) | (data[1 * 64 + 32 + 2] << 16) | (data[1 * 64 + 32 + 3] << 24);
    // SKU is not a Spoolman ID, so we don't map it to spool_id

    // Sector 2
    // Diameter (Block 0, bytes 0-1)
    uint16_t diameter = data[2 * 64 + 0] | (data[2 * 64 + 1] << 8);
    if (diameter > 0) {
        char diaBuf[8];
        snprintf(diaBuf, sizeof(diaBuf), "%.2f", (float)diameter / 100.0f);
        output.diameter = diaBuf;
    }

    // Weight (Block 0, bytes 2-3)
    uint16_t weight = data[2 * 64 + 2] | (data[2 * 64 + 3] << 8);
    if (weight > 0) output.total_weight = std::to_string(weight);

    // Drying (Block 1, bytes 0-3)
    uint16_t dryTemp = data[2 * 64 + 16 + 0] | (data[2 * 64 + 16 + 1] << 8);
    uint16_t dryTime = data[2 * 64 + 16 + 2] | (data[2 * 64 + 16 + 3] << 8);
    if (dryTemp > 0) output.dry_temp = std::to_string(dryTemp);
    if (dryTime > 0) output.dry_time = std::to_string(dryTime);

    // Temps (Block 1)
    uint16_t hotendMax = data[2 * 64 + 16 + 4] | (data[2 * 64 + 16 + 5] << 8);
    uint16_t hotendMin = data[2 * 64 + 16 + 6] | (data[2 * 64 + 16 + 7] << 8);
    if (hotendMin > 0) output.min_temp = std::to_string(hotendMin);
    if (hotendMax > 0) output.max_temp = std::to_string(hotendMax);

    uint16_t bedTemp = data[2 * 64 + 16 + 10] | (data[2 * 64 + 16 + 11] << 8);
    if (bedTemp > 0) {
        output.bed_min_temp = std::to_string(bedTemp);
        output.bed_max_temp = std::to_string(bedTemp);
    }



    // Hardware UID
    if (uid) {
        char uidBuf[13]; 
        snprintf(uidBuf, sizeof(uidBuf), "%02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3]);
        output.hardware_uid = uidBuf;
        output.lot_nr = uidBuf; // Use UID as Lot Number for linking
#ifdef ESP32
        Serial.print("Snapmaker Tag UID: ");
        Serial.println(uidBuf);
#endif
    }

    return true;
}

std::string SnapmakerTagParser::mapMainType(uint16_t typeId) {
    switch (typeId) {
        case 1: return "PLA";
        case 2: return "PETG";
        case 3: return "ABS";
        case 4: return "TPU";
        case 5: return "PVA";
        default: return "Unknown";
    }
}

std::string SnapmakerTagParser::mapSubType(uint16_t subtypeId) {
    switch (subtypeId) {
        case 1: return "Basic";
        case 2: return "Matte";
        case 3: return "SnapSpeed";
        case 4: return "Silk";
        case 5: return "Support";
        case 6: return "HF";
        case 7: return "95A";
        case 8: return "95A HF";
        default: return "";
    }
}
