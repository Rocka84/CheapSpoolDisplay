#ifndef USE_SDL2
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

bool OpenPrintTagParser::parse(const std::vector<uint8_t>& payload, OpenSpoolData& data) {
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
            decodeMainSection(&mainIt, data);
        }
    } else {
        cbor_value_advance(&it);
        if (!cbor_value_at_end(&it) && cbor_value_is_map(&it)) {
            decodeMainSection(&it, data);
        }
    }

    // 3. Parse Auxiliary Section
    if (auxOffset > 0 && auxOffset < payload.size()) {
        CborValue auxIt;
        if (cbor_parser_init(payload.data() + auxOffset, payload.size() - auxOffset, 0, &parser, &auxIt) == CborNoError) {
            decodeAuxSection(&auxIt, data);
        }
    }

    data.protocol = "openprinttag";
    return true;
}

bool OpenPrintTagParser::decodeMainSection(CborValue* it, OpenSpoolData& data) {
    if (!cbor_value_is_map(it)) return false;
    
    CborValue map;
    cbor_value_enter_container(it, &map);
    
    while (!cbor_value_at_end(&map)) {
        int64_t key;
        if (!cbor_value_is_integer(&map)) break;
        cbor_value_get_int64(&map, &key);
        cbor_value_advance(&map);
        
        CborType type = cbor_value_get_type(&map);
        
        switch (key) {
            case 0: { // instance_uuid
                if (type == CborByteStringType) {
                    size_t len;
                    uint8_t buf[16];
                    if (cbor_value_copy_byte_string(&map, buf, &len, nullptr) == CborNoError && len == 16) {
                        char hex[33];
                        for(int i=0; i<16; i++) sprintf(hex + i*2, "%02x", buf[i]);
                        data.spool_id = hex;
                    }
                }
                break;
            }
            case 9: { // material_type (enum)
                if (type == CborIntegerType && data.type.empty()) {
                    int64_t val; cbor_value_get_int64(&map, &val);
                    data.type = enumToMaterialType((int)val);
                }
                break;
            }
            case 10: { // material_name
                if (type == CborTextStringType) {
                    size_t len;
                    char buf[128];
                    if (cbor_value_copy_text_string(&map, buf, &len, nullptr) == CborNoError) {
                        data.type = std::string(buf, len);
                    }
                }
                break;
            }
            case 11: { // brand_name
                if (type == CborTextStringType) {
                    size_t len;
                    char buf[128];
                    if (cbor_value_copy_text_string(&map, buf, &len, nullptr) == CborNoError) {
                        data.brand = std::string(buf, len);
                    }
                }
                break;
            }
            case 52: { // material_abbreviation
                if (type == CborTextStringType) {
                    size_t len;
                    char buf[32];
                    if (cbor_value_copy_text_string(&map, buf, &len, nullptr) == CborNoError) {
                        data.subtype = std::string(buf, len);
                    }
                }
                break;
            }
            case 19: { // primary_color
                if (type == CborByteStringType) {
                    size_t len;
                    uint8_t buf[4];
                    if (cbor_value_copy_byte_string(&map, buf, &len, nullptr) == CborNoError && len >= 3) {
                        std::stringstream ss;
                        ss << "#" << std::hex << std::setw(2) << std::setfill('0') << (int)buf[0]
                           << std::setw(2) << std::setfill('0') << (int)buf[1]
                           << std::setw(2) << std::setfill('0') << (int)buf[2];
                        data.color_hex = ss.str();
                    }
                } else if (type == CborIntegerType) {
                    uint64_t rgb;
                    cbor_value_get_uint64(&map, &rgb);
                    std::stringstream ss;
                    ss << "#" << std::hex << std::setw(6) << std::setfill('0') << (uint32_t)rgb;
                    data.color_hex = ss.str();
                }
                break;
            }
            case 30: { // filament_diameter
                double dia = 1.75;
                if (type == CborDoubleType) cbor_value_get_double_safe(&map, &dia);
                else if (type == CborFloatType) { float f; cbor_value_get_float_safe(&map, &f); dia = f; }
                else if (type == CborIntegerType) { int64_t i; cbor_value_get_int64(&map, &i); dia = (double)i; }
                data.diameter = std::to_string(dia);
                break;
            }
            case 16: { // nominal_netto_full_weight
                double weight = 0;
                if (type == CborDoubleType) cbor_value_get_double_safe(&map, &weight);
                else if (type == CborFloatType) { float f; cbor_value_get_float_safe(&map, &f); weight = f; }
                else if (type == CborIntegerType) { int64_t i; cbor_value_get_int64(&map, &i); weight = (double)i; }
                data.total_weight = std::to_string((int)weight);
                break;
            }
            case 34: { // min_print_temperature
                if (type == CborIntegerType) {
                    int64_t t; cbor_value_get_int64(&map, &t);
                    data.min_temp = std::to_string(t);
                }
                break;
            }
            case 35: { // max_print_temperature
                if (type == CborIntegerType) {
                    int64_t t; cbor_value_get_int64(&map, &t);
                    data.max_temp = std::to_string(t);
                }
                break;
            }
            case 37: { // min_bed_temperature
                if (type == CborIntegerType) {
                    int64_t t; cbor_value_get_int64(&map, &t);
                    data.bed_min_temp = std::to_string(t);
                }
                break;
            }
            case 38: { // max_bed_temperature
                if (type == CborIntegerType) {
                    int64_t t; cbor_value_get_int64(&map, &t);
                    data.bed_max_temp = std::to_string(t);
                }
                break;
            }
        }
        cbor_value_advance(&map);
    }
    cbor_value_leave_container(it, &map);
    return true;
}

bool OpenPrintTagParser::decodeAuxSection(CborValue* it, OpenSpoolData& data) {
    if (!cbor_value_is_map(it)) return false;
    CborValue map;
    cbor_value_enter_container(it, &map);
    while (!cbor_value_at_end(&map)) {
        int64_t key;
        if (!cbor_value_is_integer(&map)) break;
        cbor_value_get_int64(&map, &key);
        cbor_value_advance(&map);
        
        CborType type = cbor_value_get_type(&map);
        
        if (key == 0) { // consumed_netto_weight
            double consumed = 0;
            if (type == CborDoubleType) cbor_value_get_double_safe(&map, &consumed);
            else if (type == CborFloatType) { float f; cbor_value_get_float_safe(&map, &f); consumed = f; }
            else if (type == CborIntegerType) { int64_t i; cbor_value_get_int64(&map, &i); consumed = (double)i; }
            
            float total = std::stof(data.total_weight.empty() ? "0" : data.total_weight);
            data.remaining_weight = std::to_string((int)(total - consumed));
        }
        cbor_value_advance(&map);
    }
    cbor_value_leave_container(it, &map);
    return true;
}

std::vector<uint8_t> OpenPrintTagParser::generate(const OpenSpoolData& data) {
    uint8_t buf[512];
    memset(buf, 0, sizeof(buf));
    CborEncoder encoder, meta, main, aux;
    
    const uint32_t mainOff = 16;
    const uint32_t auxOff = 200;

    // 1. Meta Section (at offset 0)
    cbor_encoder_init(&encoder, buf, sizeof(buf), 0);
    cbor_encoder_create_map(&encoder, &meta, CborIndefiniteLength);
    cbor_encode_int(&meta, 0); // main_region_offset
    cbor_encode_uint(&meta, mainOff);
    cbor_encode_int(&meta, 2); // aux_region_offset
    cbor_encode_uint(&meta, auxOff);
    cbor_encoder_close_container(&encoder, &meta);

    // 2. Main Section (at offset 16)
    cbor_encoder_init(&encoder, buf + mainOff, sizeof(buf) - mainOff, 0);
    cbor_encoder_create_map(&encoder, &main, CborIndefiniteLength);
    
    // REQUIRED: material_class (8) = FFF (0)
    cbor_encode_int(&main, 8); cbor_encode_int(&main, 0);
    
    // RECOMMENDED: material_type (9)
    int matType = materialTypeToEnum(data.type);
    if (matType >= 0) { cbor_encode_int(&main, 9); cbor_encode_int(&main, matType); }
    
    // material_name (10) and brand_name (11)
    cbor_encode_int(&main, 10); cbor_encode_text_stringz(&main, data.type.c_str());
    cbor_encode_int(&main, 11); cbor_encode_text_stringz(&main, data.brand.c_str());
    
    uint32_t color = 0;
    if (data.color_hex.length() >= 6) {
        color = strtol(data.color_hex.substr(data.color_hex.length()-6).c_str(), nullptr, 16);
    }
    uint8_t rgb[3] = { (uint8_t)((color >> 16) & 0xFF), (uint8_t)((color >> 8) & 0xFF), (uint8_t)(color & 0xFF) };
    cbor_encode_int(&main, 19); cbor_encode_byte_string(&main, rgb, 3);
    
    cbor_encode_int(&main, 30); cbor_encode_float(&main, std::stof(data.diameter.empty() ? "1.75" : data.diameter));
    cbor_encode_int(&main, 16); cbor_encode_float(&main, std::stof(data.total_weight.empty() ? "1000" : data.total_weight));
    
    if (!data.min_temp.empty()) { cbor_encode_int(&main, 34); cbor_encode_int(&main, std::stoi(data.min_temp)); }
    if (!data.max_temp.empty()) { cbor_encode_int(&main, 35); cbor_encode_int(&main, std::stoi(data.max_temp)); }
    if (!data.bed_min_temp.empty()) { cbor_encode_int(&main, 37); cbor_encode_int(&main, std::stoi(data.bed_min_temp)); }
    if (!data.bed_max_temp.empty()) { cbor_encode_int(&main, 38); cbor_encode_int(&main, std::stoi(data.bed_max_temp)); }
    
    cbor_encoder_close_container(&encoder, &main);

    // 3. Aux Section (at offset 200)
    cbor_encoder_init(&encoder, buf + auxOff, sizeof(buf) - auxOff, 0);
    cbor_encoder_create_map(&encoder, &aux, CborIndefiniteLength);
    cbor_encode_int(&aux, 0); // consumed_netto_weight
    float total = std::stof(data.total_weight.empty() ? "1000" : data.total_weight);
    float remaining = std::stof(data.remaining_weight.empty() ? data.total_weight : data.remaining_weight);
    cbor_encode_float(&aux, (float)(total - remaining));
    cbor_encoder_close_container(&encoder, &aux);

    size_t auxSize = cbor_encoder_get_buffer_size(&encoder, buf + auxOff);
    return std::vector<uint8_t>(buf, buf + auxOff + auxSize);
}
#else
// Stubs for simulator
#include "OpenPrintTag.h"
#include <string>
bool OpenPrintTagParser::parse(const std::vector<uint8_t>& payload, OpenSpoolData& data) { return false; }
std::vector<uint8_t> OpenPrintTagParser::generate(const OpenSpoolData& data) { return {}; }
#endif
