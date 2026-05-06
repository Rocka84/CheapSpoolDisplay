#include "../../src/data/OpenSpool.cpp"
#include "../../src/data/OpenSpool.h"
#include "../../src/network/WebhookFormatter.cpp"
#include "../../src/network/WebhookFormatter.h"
#include "../../src/data/OpenTag3D.cpp"
#include "../../src/data/OpenTag3D.h"
#include <string>
#include <unity.h>

// Define unity functions setup and teardown
void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_openspool_empty_initialization(void) {
  OpenSpoolData data;
  TEST_ASSERT_EQUAL_STRING("openspool", data.protocol.c_str());
  TEST_ASSERT_EQUAL_STRING("1", data.version.c_str());
  TEST_ASSERT_EQUAL_STRING("", data.type.c_str());
  TEST_ASSERT_EQUAL_STRING("", data.color_hex.c_str());
  TEST_ASSERT_EQUAL_STRING("", data.brand.c_str());
  TEST_ASSERT_EQUAL_STRING("", data.spool_id.c_str());
}

void test_openspool_serialization(void) {
  OpenSpoolData data;
  data.type = "PLA";
  data.color_hex = "#FF0000";
  data.brand = "PrusaMent";
  data.spool_id = "12345";

  std::string json = OpenSpoolParser::toJson(data);

  // Check if it contains the keys and values
  TEST_ASSERT_TRUE(json.find("\"protocol\":\"openspool\"") !=
                   std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"type\":\"PLA\"") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"color_hex\":\"#FF0000\"") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"brand\":\"PrusaMent\"") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"spool_id\":\"12345\"") != std::string::npos);
}

void test_openspool_deserialization_valid(void) {
  std::string valid_json =
      "{\"protocol\":\"openspool\",\"version\":\"1\",\"type\":\"PETG\",\"color_"
      "hex\":\"#00FF00\",\"brand\":\"Sunlu\"}";
  OpenSpoolData data;

  bool result = OpenSpoolParser::parseJson(valid_json, data);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL_STRING("openspool", data.protocol.c_str());
  TEST_ASSERT_EQUAL_STRING("1", data.version.c_str());
  TEST_ASSERT_EQUAL_STRING("PETG", data.type.c_str());
  TEST_ASSERT_EQUAL_STRING("#00FF00", data.color_hex.c_str());
  TEST_ASSERT_EQUAL_STRING("Sunlu", data.brand.c_str());
  TEST_ASSERT_EQUAL_STRING(
      "", data.spool_id.c_str()); // Should be empty since it was missing
}

void test_openspool_deserialization_invalid_protocol(void) {
  std::string invalid_json =
      "{\"protocol\":\"wrongprotocol\",\"version\":\"1\",\"type\":\"ABS\"}";
  OpenSpoolData data;

  bool result = OpenSpoolParser::parseJson(invalid_json, data);

  TEST_ASSERT_FALSE(result); // Should fail because protocol != openspool
}

void test_openspool_deserialization_bad_json(void) {
  std::string bad_json = "{bad_json: 123";
  OpenSpoolData data;

  bool result = OpenSpoolParser::parseJson(bad_json, data);

  TEST_ASSERT_FALSE(result);
}

void test_webhook_format_no_placeholders(void) {
  std::string url = "http://example.com/api";
  TEST_ASSERT_EQUAL_STRING("http://example.com/api",
                           WebhookFormatter::formatUrl(url, "123", 4).c_str());
}

void test_webhook_format_spool_id(void) {
  std::string url = "http://example.com/api?spool={spool_id}";
  TEST_ASSERT_EQUAL_STRING("http://example.com/api?spool=123",
                           WebhookFormatter::formatUrl(url, "123", 4).c_str());
}

void test_webhook_format_toolhead(void) {
  std::string url = "http://example.com/api?tool={toolhead}";
  TEST_ASSERT_EQUAL_STRING("http://example.com/api?tool=4",
                           WebhookFormatter::formatUrl(url, "a", 4).c_str());
}

void test_webhook_format_both(void) {
  std::string url = "http://example.com/api?spool={spool_id}&tool={toolhead}";
  TEST_ASSERT_EQUAL_STRING(
      "http://example.com/api?spool=abc-def&tool=2",
      WebhookFormatter::formatUrl(url, "abc-def", 2).c_str());
}

void test_openspool_color_normalization(void) {
  std::string json = "{\"protocol\":\"openspool\",\"color_hex\":\"FF0000\"}";
  OpenSpoolData data;
  OpenSpoolParser::parseJson(json, data);
  TEST_ASSERT_EQUAL_STRING("#FF0000", data.color_hex.c_str());

  json = "{\"protocol\":\"openspool\",\"color_hex\":\"#00FF00\"}";
  OpenSpoolParser::parseJson(json, data);
  TEST_ASSERT_EQUAL_STRING("#00FF00", data.color_hex.c_str());
}

void test_openspool_spool_id_types(void) {
  std::string json = "{\"protocol\":\"openspool\",\"spool_id\":12345}";
  OpenSpoolData data;
  OpenSpoolParser::parseJson(json, data);
  TEST_ASSERT_EQUAL_STRING("12345", data.spool_id.c_str());

  json = "{\"protocol\":\"openspool\",\"spool_id\":\"abc-789\"}";
  OpenSpoolParser::parseJson(json, data);
  TEST_ASSERT_EQUAL_STRING("abc-789", data.spool_id.c_str());
}

void test_openspool_enrichment_valid(void) {
  std::string json = "{"
                     "\"filament\":{\"name\":\"Galaxy Black\"},"
                     "\"remaining_weight\":450.5,"
                     "\"initial_weight\":750,"
                     "\"spool_weight\":250"
                     "}";
  OpenSpoolData data;
  bool result = OpenSpoolParser::enrichFromSpoolman(json, data);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL_STRING("Galaxy Black", data.filament_name.c_str());
  TEST_ASSERT_EQUAL_STRING("450.5g", data.remaining_weight.c_str());
  TEST_ASSERT_EQUAL_STRING("1,000g", data.total_weight.c_str());
}

void test_openspool_enrichment_partial(void) {
  std::string json = "{\"filament\":{\"name\":\"Only Name\"}}";
  OpenSpoolData data;
  bool result = OpenSpoolParser::enrichFromSpoolman(json, data);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL_STRING("Only Name", data.filament_name.c_str());
  TEST_ASSERT_EQUAL_STRING("", data.remaining_weight.c_str());
}

void test_webhook_format_multi_placeholders(void) {
  std::string url = "http://api.com/{spool_id}/{spool_id}/{toolhead}";
  std::string result = WebhookFormatter::formatUrl(url, "abc", 1);
  TEST_ASSERT_EQUAL_STRING("http://api.com/abc/abc/1", result.c_str());
}

void test_webhook_format_malformed_placeholder(void) {
  std::string url = "http://api.com/{spool_id"; // No closing brace
  std::string result = WebhookFormatter::formatUrl(url, "abc", 1);
  TEST_ASSERT_EQUAL_STRING("http://api.com/{spool_id", result.c_str());
}

void test_openspool_deserialization_no_protocol(void) {
  std::string json = "{\"version\":\"1\",\"type\":\"PLA\"}";
  OpenSpoolData data;
  bool result = OpenSpoolParser::parseJson(json, data);
  TEST_ASSERT_FALSE(result);
}

void test_opentag3d_parsing(void) {
    std::vector<uint8_t> payload(200, 0);
    // Version 1.000 (1000 = 0x03E8)
    payload[0x00] = 0x03;
    payload[0x01] = 0xE8;
    // Material "PLA  "
    memcpy(&payload[0x02], "PLA  ", 5);
    // Brand "Prusa           "
    memcpy(&payload[0x1B], "Prusa           ", 16);
    // Color Name "Galaxy Black                    "
    memcpy(&payload[0x2B], "Galaxy Black                    ", 32);
    // Color RGBA (255, 10, 20, 128) -> FF, 0A, 14, 80
    payload[0x4B] = 0xFF;
    payload[0x4C] = 0x0A;
    payload[0x4D] = 0x14;
    payload[0x4E] = 0x80;
    // Nozzle 210C -> 210/5 = 42 (0x2A)
    payload[0x60] = 0x2A;
    // URL with ID
    memcpy(&payload[0x70], "spoolman.local/spool/show/1234  ", 32);

    OpenSpoolData data;
    bool result = OpenTag3DParser::parseBinary(payload, data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("opentag3d", data.protocol.c_str());
    TEST_ASSERT_EQUAL_STRING("PLA", data.type.c_str());
    TEST_ASSERT_EQUAL_STRING("Prusa", data.brand.c_str());
    TEST_ASSERT_EQUAL_STRING("Galaxy Black", data.filament_name.c_str());
    TEST_ASSERT_EQUAL_STRING("#FF0A14", data.color_hex.c_str());
    TEST_ASSERT_EQUAL_STRING("80", data.alpha.c_str());
    TEST_ASSERT_EQUAL_STRING("210", data.min_temp.c_str());
    TEST_ASSERT_EQUAL_STRING("1234", data.spool_id.c_str());
    TEST_ASSERT_EQUAL_STRING("", data.lot_nr.c_str());
}

void test_opentag3d_roundtrip() {
    OpenSpoolData original;
    original.brand = "BrandX";
    original.type = "PLA";
    original.subtype = "Silk";
    original.filament_name = "Shiny Blue";
    original.color_hex = "#112233";
    original.alpha = "FF";
    original.max_temp = "215";
    original.bed_max_temp = "60";
    original.total_weight = "1000";
    original.spool_id = "12345";
    original.diameter = "1.750";

    std::vector<uint8_t> binary = OpenTag3DParser::generateBinary(original);
    
    OpenSpoolData parsed;
    bool success = OpenTag3DParser::parseBinary(binary, parsed);
    
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("BrandX", parsed.brand.c_str());
    TEST_ASSERT_EQUAL_STRING("PLA", parsed.type.c_str());
    TEST_ASSERT_EQUAL_STRING("Silk", parsed.subtype.c_str());
    TEST_ASSERT_EQUAL_STRING("Shiny Blue", parsed.filament_name.c_str());
    TEST_ASSERT_EQUAL_STRING("#112233", parsed.color_hex.c_str());
    TEST_ASSERT_EQUAL_STRING("FF", parsed.alpha.c_str());
    TEST_ASSERT_EQUAL_STRING("215", parsed.max_temp.c_str());
    TEST_ASSERT_EQUAL_STRING("60", parsed.bed_max_temp.c_str());
    TEST_ASSERT_EQUAL_STRING("1000", parsed.total_weight.c_str());
    TEST_ASSERT_EQUAL_STRING("12345", parsed.spool_id.c_str());
    TEST_ASSERT_EQUAL_STRING("1.750", parsed.diameter.c_str());
    TEST_ASSERT_EQUAL_STRING("", parsed.lot_nr.c_str());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_openspool_empty_initialization);
  RUN_TEST(test_openspool_serialization);
  RUN_TEST(test_openspool_deserialization_valid);
  RUN_TEST(test_openspool_deserialization_invalid_protocol);
  RUN_TEST(test_openspool_deserialization_bad_json);
  RUN_TEST(test_openspool_deserialization_no_protocol);
  RUN_TEST(test_openspool_color_normalization);
  RUN_TEST(test_openspool_spool_id_types);
  RUN_TEST(test_openspool_enrichment_valid);
  RUN_TEST(test_openspool_enrichment_partial);
  RUN_TEST(test_webhook_format_no_placeholders);
  RUN_TEST(test_webhook_format_spool_id);
  RUN_TEST(test_webhook_format_toolhead);
  RUN_TEST(test_webhook_format_both);
  RUN_TEST(test_webhook_format_multi_placeholders);
  RUN_TEST(test_webhook_format_malformed_placeholder);
  RUN_TEST(test_opentag3d_parsing);
  RUN_TEST(test_opentag3d_roundtrip);
  UNITY_END();

  return 0;
}
