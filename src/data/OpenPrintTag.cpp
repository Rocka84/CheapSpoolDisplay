#include "OpenPrintTag.h"
#include <iomanip>
#include <sstream>

// Workaround for TinyCBOR header assert bug on some platforms
// Some versions of cbor.h have strict asserts on flags that are not always set by the parser for floats/doubles
static CborError cbor_value_get_double_safe(const CborValue* value, double* result) {
    if (value->type != CborDoubleType) return CborErrorIllegalType;
    CborValue v = *value;
    v.flags |= 0x02; // CborIteratorFlag_IntegerValueTooLarge
    v.flags |= 0x01; // CborIteratorFlag_IntegerValueIs64Bit
    return cbor_value_get_double(&v, result);
}

static CborError cbor_value_get_float_safe(const CborValue* value, float* result) {
    if (value->type != CborFloatType) return CborErrorIllegalType;
    CborValue v = *value;
    v.flags |= 0x02; // CborIteratorFlag_IntegerValueTooLarge
    v.flags &= ~0x01; // Not 64-bit
    return cbor_value_get_float(&v, result);
}

static int materialTypeToEnum(const std::string& type) {
    std::string t = type;
    for (auto & c: t) c = toupper(c);
    if (t == "PLA") return 0;
    if (t == "PETG") return 1;
    if (t == "TPU") return 2;
    if (t == "ABS") return 3;
    if (t == "ASA") return 4;
    if (t == "PC") return 5;
    if (t == "PA" || t == "NYLON") return 10;
    return -1;
}

static std::string enumToMaterialType(int key) {
    switch (key) {
        case 0: return "PLA";
        case 1: return "PETG";
        case 2: return "TPU";
        case 3: return "ABS";
        case 4: return "ASA";
        case 5: return "PC";
        case 10: return "PA";
        default: return "";
    }
}

bool OpenPrintTagParser::parse(const std::vector<uint8_t>& payload, OpenSpoolData& spoolData) {
    if (payload.empty()) return false;

    CborParser parser;
    CborValue it;
    if (cbor_parser_init(payload.data(), payload.size(), 0, &parser, &it) != CborNoError) {
        return false;
    }

    if (!cbor_value_is_map(&it)) return false;

    // 1. Parse Meta Section (always at offset 0)
    uint64_t mainOffset = 0;
    uint64_t auxOffset = 0;
    
    CborValue metaMap;
    cbor_value_enter_container(&it, &metaMap);
    while (!cbor_value_at_end(&metaMap)) {
        int64_t key;
        if (cbor_value_get_int64(&metaMap, &key) == CborNoError) {
            cbor_value_advance(&metaMap);
            if (key == 0) cbor_value_get_uint64(&metaMap, &mainOffset);
            else if (key == 2) cbor_value_get_uint64(&metaMap, &auxOffset);
            cbor_value_advance(&metaMap);
        } else break;
    }

    // 2. Parse Main Section
    if (mainOffset > 0 && mainOffset < payload.size()) {
        CborValue mainIt;
        if (cbor_parser_init(payload.data() + mainOffset, payload.size() - mainOffset, 0, &parser, &mainIt) == CborNoError) {
            decodeMainSection(&mainIt, spoolData);
        }
    } else {
        cbor_value_advance(&it);
        if (!cbor_value_at_end(&it) && cbor_value_is_map(&it)) {
            decodeMainSection(&it, spoolData);
        }
    }

    // 3. Parse Auxiliary Section
    if (auxOffset > 0 && auxOffset < payload.size()) {
        CborValue auxIt;
        if (cbor_parser_init(payload.data() + auxOffset, payload.size() - auxOffset, 0, &parser, &auxIt) == CborNoError) {
            decodeAuxSection(&auxIt, spoolData);
        }
    }

    spoolData.protocol = "openprinttag";
    return true;
}

bool OpenPrintTagParser::decodeMainSection(CborValue* it, OpenSpoolData& spoolData) {
    if (!cbor_value_is_map(it)) return false;
    
    CborValue map;
    cbor_value_enter_container(it, &map);
    
    while (!cbor_value_at_end(&map)) {
        if (!cbor_value_is_integer(&map)) {
            cbor_value_advance(&map);
            if (!cbor_value_at_end(&map)) cbor_value_advance(&map);
            continue;
        }
        int64_t key;
        cbor_value_get_int64(&map, &key);
        cbor_value_advance(&map);
        
        CborType type = cbor_value_get_type(&map);
        bool manualAdvance = false;
        
        switch (key) {
            case 0: { // instance_uuid
                if (type == CborByteStringType) {
                    size_t len = 16;
                    uint8_t buf[16];
                    if (cbor_value_copy_byte_string(&map, buf, &len, nullptr) == CborNoError && len == 16) {
                        // Check for Spoolman prefix "SPMN" (53 50 4d 4e)
                        if (buf[0] == 0x53 && buf[1] == 0x50 && buf[2] == 0x4d && buf[3] == 0x4e) {
                            // Extract integer from last 8 bytes
                            uint64_t id = 0;
                            for (int i = 8; i < 16; i++) id = (id << 8) | buf[i];
                            spoolData.spool_id = std::to_string(id);
                        } else {
                            char hex[33];
                            for(int i=0; i<16; i++) sprintf(hex + i*2, "%02x", buf[i]);
                            spoolData.spool_id = hex;
                        }
                    }
                } else if (type == CborTextStringType) {
                    size_t len = 64;
                    char buf[64];
                    if (cbor_value_copy_text_string(&map, buf, &len, nullptr) == CborNoError) {
                        spoolData.spool_id = std::string(buf, len);
                    }
                } else if (type == CborIntegerType) {
                    int64_t val; cbor_value_get_int64(&map, &val);
                    spoolData.spool_id = std::to_string(val);
                }
                break;
            }
            case 9: { // material_type (enum)
                if (type == CborIntegerType && spoolData.type.empty()) {
                    int64_t val; cbor_value_get_int64(&map, &val);
                    spoolData.type = enumToMaterialType((int)val);
                }
                break;
            }
            case 10: { // material_name
                if (type == CborTextStringType) {
                    size_t len = 128;
                    char buf[128];
                    if (cbor_value_copy_text_string(&map, buf, &len, nullptr) == CborNoError) {
                        spoolData.filament_name = std::string(buf, len);
                    }
                } else if (type == CborIntegerType) {
                    int64_t val; cbor_value_get_int64(&map, &val);
                    spoolData.filament_name = std::to_string(val);
                }
                break;
            }
            case 11: { // brand_name
                if (type == CborTextStringType) {
                    size_t len = 128;
                    char buf[128];
                    if (cbor_value_copy_text_string(&map, buf, &len, nullptr) == CborNoError) {
                        spoolData.brand = std::string(buf, len);
                    }
                } else if (type == CborIntegerType) {
                    int64_t val; cbor_value_get_int64(&map, &val);
                    spoolData.brand = std::to_string(val);
                }
                break;
            }
            case 52: { // material_abbreviation
                if (type == CborTextStringType) {
                    size_t len = 32;
                    char buf[32];
                    if (cbor_value_copy_text_string(&map, buf, &len, nullptr) == CborNoError) {
                        spoolData.subtype = std::string(buf, len);
                    }
                }
                break;
            }
            case 19: { // primary_color
                if (type == CborByteStringType) {
                    size_t len = 4;
                    uint8_t buf[4];
                    if (cbor_value_copy_byte_string(&map, buf, &len, nullptr) == CborNoError && len >= 3) {
                        std::stringstream ss;
                        ss << "#" << std::hex << std::setw(2) << std::setfill('0') << (int)buf[0]
                           << std::setw(2) << std::setfill('0') << (int)buf[1]
                           << std::setw(2) << std::setfill('0') << (int)buf[2];
                        spoolData.color_hex = ss.str();
                    }
                } else if (type == CborIntegerType) {
                    uint64_t rgb;
                    cbor_value_get_uint64(&map, &rgb);
                    std::stringstream ss;
                    ss << "#" << std::hex << std::setw(6) << std::setfill('0') << (uint32_t)rgb;
                    spoolData.color_hex = ss.str();
                }
                break;
            }
            case 30: { // filament_diameter
                double dia = 1.75;
                if (type == CborDoubleType) cbor_value_get_double_safe(&map, &dia);
                else if (type == CborFloatType) { float f; cbor_value_get_float_safe(&map, &f); dia = f; }
                else if (type == CborIntegerType) { int64_t i; cbor_value_get_int64(&map, &i); dia = (double)i; }
                spoolData.diameter = std::to_string(dia);
                break;
            }
            case 16: { // nominal_netto_full_weight
                double weight = 0;
                if (type == CborDoubleType) cbor_value_get_double_safe(&map, &weight);
                else if (type == CborFloatType) { float f; cbor_value_get_float_safe(&map, &f); weight = f; }
                else if (type == CborIntegerType) { int64_t i; cbor_value_get_int64(&map, &i); weight = (double)i; }
                spoolData.total_weight = std::to_string((int)weight);
                break;
            }
            case 34: { // min_print_temperature
                if (type == CborIntegerType) {
                    int64_t t; cbor_value_get_int64(&map, &t);
                    spoolData.min_temp = std::to_string(t);
                }
                break;
            }
            case 35: { // max_print_temperature
                if (type == CborIntegerType) {
                    int64_t t; cbor_value_get_int64(&map, &t);
                    spoolData.max_temp = std::to_string(t);
                }
                break;
            }
            case 37: { // min_bed_temperature
                if (type == CborIntegerType) {
                    int64_t t; cbor_value_get_int64(&map, &t);
                    spoolData.bed_min_temp = std::to_string(t);
                }
                break;
            }
            case 38: { // max_bed_temperature
                if (type == CborIntegerType) {
                    int64_t t; cbor_value_get_int64(&map, &t);
                    spoolData.bed_max_temp = std::to_string(t);
                }
                break;
            }
            case 17: { // actual_netto_full_weight
                double w = 0;
                if (type == CborDoubleType) cbor_value_get_double_safe(&map, &w);
                else if (type == CborFloatType) { float f; cbor_value_get_float_safe(&map, &f); w = f; }
                else if (type == CborIntegerType) { int64_t i; cbor_value_get_int64(&map, &i); w = (double)i; }
                spoolData.actual_weight = std::to_string((int)w);
                break;
            }
            case 27: { // transmission_distance
                double td = 0;
                if (type == CborDoubleType) cbor_value_get_double_safe(&map, &td);
                else if (type == CborFloatType) { float f; cbor_value_get_float_safe(&map, &f); td = f; }
                else if (type == CborIntegerType) { int64_t i; cbor_value_get_int64(&map, &i); td = (double)i; }
                char buf[16]; snprintf(buf, sizeof(buf), "%.2f", td);
                spoolData.td = buf;
                break;
            }
            case 28: { // tags (enum_array)
                if (type == CborArrayType) {
                    spoolData.tags = "CBOR Array"; // Simplified for now
                }
                break;
            }
            case 31: { // shore_hardness_a
                if (type == CborIntegerType) {
                    int64_t h; cbor_value_get_int64(&map, &h);
                    spoolData.shore = std::to_string(h) + "A";
                }
                break;
            }
            case 57: { // drying_temperature
                if (type == CborIntegerType) {
                    int64_t t; cbor_value_get_int64(&map, &t);
                    spoolData.dry_temp = std::to_string(t);
                }
                break;
            }
            case 58: { // drying_time
                if (type == CborIntegerType) {
                    int64_t t; cbor_value_get_int64(&map, &t);
                    spoolData.dry_time = std::to_string(t);
                }
                break;
            }
        }
        
        if (!manualAdvance && !cbor_value_at_end(&map)) {
            cbor_value_advance(&map);
        }
    }
    cbor_value_leave_container(it, &map);
    return true;
}

bool OpenPrintTagParser::decodeAuxSection(CborValue* it, OpenSpoolData& spoolData) {
    if (!cbor_value_is_map(it)) return false;
    CborValue map;
    cbor_value_enter_container(it, &map);
    while (!cbor_value_at_end(&map)) {
        if (!cbor_value_is_integer(&map)) {
            cbor_value_advance(&map);
            if (!cbor_value_at_end(&map)) cbor_value_advance(&map);
            continue;
        }
        int64_t key;
        cbor_value_get_int64(&map, &key);
        cbor_value_advance(&map);
        
        CborType type = cbor_value_get_type(&map);
        bool manualAdvance = false;
        
        if (key == 0) { // consumed_netto_weight
            double consumed = 0;
            if (type == CborDoubleType) cbor_value_get_double_safe(&map, &consumed);
            else if (type == CborFloatType) { float f; cbor_value_get_float_safe(&map, &f); consumed = f; }
            else if (type == CborIntegerType) { int64_t i; cbor_value_get_int64(&map, &i); consumed = (double)i; }
            
            float total = std::stof(spoolData.total_weight.empty() ? "0" : spoolData.total_weight);
            spoolData.remaining_weight = std::to_string((int)(total - consumed));
        } else if (key == 4) { // storage_location
            if (type == CborTextStringType) {
                size_t len = 64;
                char buf[64];
                if (cbor_value_copy_text_string(&map, buf, &len, nullptr) == CborNoError) {
                    spoolData.location = std::string(buf, len);
                }
            }
        }
        if (!manualAdvance && !cbor_value_at_end(&map)) {
            cbor_value_advance(&map);
        }
    }
    cbor_value_leave_container(it, &map);
    return true;
}

std::vector<uint8_t> OpenPrintTagParser::generate(const OpenSpoolData& spoolData) {
    uint8_t buf[512];
    memset(buf, 0, sizeof(buf));
    CborEncoder encoder, meta, main, aux;
    
    // 1. Prepare Main Section first to know its size
    uint8_t mainBuf[384];
    cbor_encoder_init(&encoder, mainBuf, sizeof(mainBuf), 0);
    
    // Count main fields
    size_t mainCount = 4; // material_class(8), material_name(10), brand_name(11), color(19)
    if (!spoolData.spool_id.empty()) mainCount++; // instance_uuid(0)
    if (!spoolData.gtin.empty()) mainCount++; // gtin(4)
    int matType = materialTypeToEnum(spoolData.type);
    if (matType >= 0) mainCount++; // material_type(9)
    if (!spoolData.diameter.empty()) mainCount++; // diameter(30)
    if (!spoolData.total_weight.empty()) mainCount++; // nominal_weight(16)
    if (!spoolData.min_temp.empty()) mainCount++; // min_print(34)
    if (!spoolData.max_temp.empty()) mainCount++; // max_print(35)
    if (!spoolData.bed_min_temp.empty()) mainCount++; // min_bed(37)
    if (!spoolData.bed_max_temp.empty()) mainCount++; // max_bed(38)
    if (!spoolData.actual_weight.empty()) mainCount++; // actual_weight(17)
    if (!spoolData.td.empty()) mainCount++; // td(27)
    if (!spoolData.shore.empty() && atoi(spoolData.shore.c_str()) > 0) mainCount++; // shore(31)
    if (!spoolData.dry_temp.empty()) mainCount++; // dry_temp(57)
    if (!spoolData.dry_time.empty()) mainCount++; // dry_time(58)
    if (!spoolData.subtype.empty()) mainCount++; // abbreviation(52)

    cbor_encoder_create_map(&encoder, &main, mainCount);
    
    // Key 0: instance_uuid
    if (!spoolData.spool_id.empty()) {
        cbor_encode_int(&main, 0);
        
        // Try to see if it's a Spoolman ID (all digits)
        bool isNumber = true;
        for (char c : spoolData.spool_id) {
            if (!isdigit(c)) { isNumber = false; break; }
        }

        if (isNumber) {
            // Encode as "SPMN" prefixed UUID: [53 50 4d 4e] [00 00 00 00] [64-bit ID]
            uint8_t uuid[16] = {0};
            uuid[0] = 0x53; uuid[1] = 0x50; uuid[2] = 0x4d; uuid[3] = 0x4e;
            uint64_t id = std::stoull(spoolData.spool_id);
            for (int i = 0; i < 8; i++) {
                uuid[15 - i] = (uint8_t)((id >> (i * 8)) & 0xFF);
            }
            cbor_encode_byte_string(&main, uuid, 16);
        } else {
            bool isHex = (spoolData.spool_id.length() == 32);
            if (isHex) {
                for (char c : spoolData.spool_id) {
                    if (!isxdigit(c)) { isHex = false; break; }
                }
            }

            if (isHex) {
                uint8_t uuid[16];
                for (int i = 0; i < 16; i++) {
                    uuid[i] = (uint8_t)strtol(spoolData.spool_id.substr(i*2, 2).c_str(), nullptr, 16);
                }
                cbor_encode_byte_string(&main, uuid, 16);
            } else {
                cbor_encode_text_stringz(&main, spoolData.spool_id.c_str());
            }
        }
    }

    // Key 4: gtin
    if (!spoolData.gtin.empty()) {
        cbor_encode_int(&main, 4);
        cbor_encode_text_stringz(&main, spoolData.gtin.c_str());
    }

    // REQUIRED: material_class (8) = FFF (0)
    cbor_encode_int(&main, 8); cbor_encode_int(&main, 0);
    
    if (matType >= 0) { cbor_encode_int(&main, 9); cbor_encode_int(&main, matType); }
    
    cbor_encode_int(&main, 10); cbor_encode_text_stringz(&main, spoolData.type.c_str());
    cbor_encode_int(&main, 11); cbor_encode_text_stringz(&main, spoolData.brand.c_str());
    
    if (!spoolData.subtype.empty()) {
        cbor_encode_int(&main, 52); cbor_encode_text_stringz(&main, spoolData.subtype.c_str());
    }

    uint32_t color = 0;
    if (spoolData.color_hex.length() >= 6) {
        color = strtol(spoolData.color_hex.substr(spoolData.color_hex.length()-6).c_str(), nullptr, 16);
    }
    uint8_t rgb[3] = { (uint8_t)((color >> 16) & 0xFF), (uint8_t)((color >> 8) & 0xFF), (uint8_t)(color & 0xFF) };
    cbor_encode_int(&main, 19); cbor_encode_byte_string(&main, rgb, 3);
    
    if (!spoolData.diameter.empty()) { cbor_encode_int(&main, 30); cbor_encode_float(&main, std::stof(spoolData.diameter)); }
    if (!spoolData.total_weight.empty()) { cbor_encode_int(&main, 16); cbor_encode_float(&main, std::stof(spoolData.total_weight)); }
    
    if (!spoolData.min_temp.empty()) { cbor_encode_int(&main, 34); cbor_encode_int(&main, std::stoi(spoolData.min_temp)); }
    if (!spoolData.max_temp.empty()) { cbor_encode_int(&main, 35); cbor_encode_int(&main, std::stoi(spoolData.max_temp)); }
    if (!spoolData.bed_min_temp.empty()) { cbor_encode_int(&main, 37); cbor_encode_int(&main, std::stoi(spoolData.bed_min_temp)); }
    if (!spoolData.bed_max_temp.empty()) { cbor_encode_int(&main, 38); cbor_encode_int(&main, std::stoi(spoolData.bed_max_temp)); }
    
    if (!spoolData.actual_weight.empty()) { cbor_encode_int(&main, 17); cbor_encode_float(&main, std::stof(spoolData.actual_weight)); }
    if (!spoolData.td.empty()) { cbor_encode_int(&main, 27); cbor_encode_float(&main, std::stof(spoolData.td)); }
    if (!spoolData.shore.empty()) { 
        int val = atoi(spoolData.shore.c_str());
        if (val > 0) { cbor_encode_int(&main, 31); cbor_encode_int(&main, val); } 
    }
    if (!spoolData.dry_temp.empty()) { cbor_encode_int(&main, 57); cbor_encode_int(&main, std::stoi(spoolData.dry_temp)); }
    if (!spoolData.dry_time.empty()) { cbor_encode_int(&main, 58); cbor_encode_int(&main, std::stoi(spoolData.dry_time)); }
    
    cbor_encoder_close_container(&encoder, &main);
    size_t mainSize = cbor_encoder_get_buffer_size(&encoder, mainBuf);

    // 2. Prepare Aux Section
    uint8_t auxBuf[128];
    cbor_encoder_init(&encoder, auxBuf, sizeof(auxBuf), 0);
    size_t auxCount = 1; // consumed_weight(0)
    if (!spoolData.location.empty()) auxCount++; // storage_location(4)
    
    cbor_encoder_create_map(&encoder, &aux, auxCount);
    cbor_encode_int(&aux, 0);
    float total = std::stof(spoolData.total_weight.empty() ? "1000" : spoolData.total_weight);
    float remaining = std::stof(spoolData.remaining_weight.empty() ? spoolData.total_weight : spoolData.remaining_weight);
    cbor_encode_float(&aux, (float)(total - remaining));
    
    if (!spoolData.location.empty()) {
        cbor_encode_int(&aux, 4);
        cbor_encode_text_stringz(&aux, spoolData.location.c_str());
    }
    cbor_encoder_close_container(&encoder, &aux);
    size_t auxSize = cbor_encoder_get_buffer_size(&encoder, auxBuf);

    // 3. Assemble with Meta Section
    // Meta uses key 0, 1, 2, 3 for mainOff, mainSize, auxOff, auxSize
    cbor_encoder_init(&encoder, buf, sizeof(buf), 0);
    cbor_encoder_create_map(&encoder, &meta, 4);
    
    uint32_t metaSize = 13; // Heuristic for {0:X, 1:X, 2:X, 3:X} with 1-byte offsets
    uint32_t mainOff = metaSize;
    uint32_t auxOff = mainOff + mainSize;
    
    cbor_encode_int(&meta, 0); cbor_encode_uint(&meta, mainOff);
    cbor_encode_int(&meta, 1); cbor_encode_uint(&meta, (uint64_t)mainSize);
    cbor_encode_int(&meta, 2); cbor_encode_uint(&meta, auxOff);
    cbor_encode_int(&meta, 3); cbor_encode_uint(&meta, (uint64_t)auxSize);
    cbor_encoder_close_container(&encoder, &meta);
    
    size_t actualMetaSize = cbor_encoder_get_buffer_size(&encoder, buf);
    if (actualMetaSize != metaSize) {
        // Recalculate if meta size was different
        mainOff = actualMetaSize;
        auxOff = mainOff + mainSize;
        cbor_encoder_init(&encoder, buf, sizeof(buf), 0);
        cbor_encoder_create_map(&encoder, &meta, 4);
        cbor_encode_int(&meta, 0); cbor_encode_uint(&meta, mainOff);
        cbor_encode_int(&meta, 1); cbor_encode_uint(&meta, (uint64_t)mainSize);
        cbor_encode_int(&meta, 2); cbor_encode_uint(&meta, auxOff);
        cbor_encode_int(&meta, 3); cbor_encode_uint(&meta, (uint64_t)auxSize);
        cbor_encoder_close_container(&encoder, &meta);
        actualMetaSize = cbor_encoder_get_buffer_size(&encoder, buf);
    }

    std::vector<uint8_t> result;
    result.assign(buf, buf + actualMetaSize);
    result.insert(result.end(), mainBuf, mainBuf + mainSize);
    result.insert(result.end(), auxBuf, auxBuf + auxSize);
    
    return result;
}
