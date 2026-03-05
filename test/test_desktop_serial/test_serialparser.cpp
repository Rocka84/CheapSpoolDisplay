#include <string>
#include <unity.h>

// To test our Serial parsing logic natively without pulling in Arduino's
// HardwareSerial, we will stub the logic from SerialTerminal.cpp to just test
// the string extraction:
void processCommand(std::string cmd, std::string &outKey,
                    std::string &outValue) {
  outKey = "";
  outValue = "";

  // Trim equivalent
  size_t first = cmd.find_first_not_of(" \t\r\n");
  if (std::string::npos == first)
    return;
  size_t last = cmd.find_last_not_of(" \t\r\n");
  cmd = cmd.substr(first, (last - first + 1));

  if (cmd.find("set ") == 0) {
    // Start searching for the key after "set "
    size_t firstSpace = cmd.find_first_not_of(" \t", 3); // Skip "set"
    if (firstSpace == std::string::npos)
      return;

    size_t keyStart = firstSpace;
    size_t keyEnd = cmd.find_first_of(" \t", keyStart);

    if (keyEnd == std::string::npos)
      return; // Missing value

    size_t valStart = cmd.find_first_not_of(" \t", keyEnd);
    if (valStart == std::string::npos)
      return; // Missing value

    std::string key = cmd.substr(keyStart, keyEnd - keyStart);
    std::string value = cmd.substr(valStart);

    // Trim key and value
    first = key.find_first_not_of(" \t\r\n");
    if (first != std::string::npos) {
      last = key.find_last_not_of(" \t\r\n");
      key = key.substr(first, (last - first + 1));
    }

    first = value.find_first_not_of(" \t\r\n");
    if (first != std::string::npos) {
      last = value.find_last_not_of(" \t\r\n");
      value = value.substr(first, (last - first + 1));
    }

    outKey = key;
    outValue = value;
  }
}

void test_serial_parser_valid_ssid() {
  std::string k, v;
  processCommand("set ssid MyHomeWiFi", k, v);
  TEST_ASSERT_EQUAL_STRING("ssid", k.c_str());
  TEST_ASSERT_EQUAL_STRING("MyHomeWiFi", v.c_str());
}

void test_serial_parser_valid_pass_with_spaces() {
  std::string k, v;
  // Password might contain spaces, we need to make sure we don't chop it early
  processCommand("set pass My Super Secret Pass", k, v);
  TEST_ASSERT_EQUAL_STRING("pass", k.c_str());
  TEST_ASSERT_EQUAL_STRING("My Super Secret Pass", v.c_str());
}

void test_serial_parser_valid_url() {
  std::string k, v;
  processCommand("set url http://192.168.1.100:5000/webhook", k, v);
  TEST_ASSERT_EQUAL_STRING("url", k.c_str());
  TEST_ASSERT_EQUAL_STRING("http://192.168.1.100:5000/webhook", v.c_str());
}

void test_serial_parser_malformed_missing_value() {
  std::string k, v;
  processCommand("set ssid", k, v);
  TEST_ASSERT_EQUAL_STRING("", k.c_str());
  TEST_ASSERT_EQUAL_STRING("", v.c_str());
}

void test_serial_parser_extra_whitespace() {
  std::string k, v;
  processCommand("   set    url    http://target.com   \n", k, v);
  TEST_ASSERT_EQUAL_STRING("url", k.c_str());
  TEST_ASSERT_EQUAL_STRING("http://target.com", v.c_str());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_serial_parser_valid_ssid);
  RUN_TEST(test_serial_parser_valid_pass_with_spaces);
  RUN_TEST(test_serial_parser_valid_url);
  RUN_TEST(test_serial_parser_malformed_missing_value);
  RUN_TEST(test_serial_parser_extra_whitespace);
  UNITY_END();
  return 0;
}
