#ifndef OPENSPOOL_H
#define OPENSPOOL_H
#ifdef USE_SDL2
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_PROGMEM 0
#endif
#include <ArduinoJson.h>
#include <string>

struct OpenSpoolData {
  std::string protocol;
  std::string version;
  std::string type;
  std::string color_hex;
  std::string brand;
  std::string spool_id;
  std::string min_temp;
  std::string max_temp;
  std::string bed_min_temp;
  std::string bed_max_temp;
  std::string lot_nr;
  std::string subtype;

  OpenSpoolData()
      : protocol("openspool"), version("1"), type(""), color_hex(""), brand(""),
        spool_id(""), min_temp(""), max_temp(""), bed_min_temp(""),
        bed_max_temp(""), lot_nr(""), subtype("") {}
};

class OpenSpoolParser {
public:
  static bool parseJson(const std::string &json, OpenSpoolData &data);
  static std::string toJson(const OpenSpoolData &data);
};

#endif // OPENSPOOL_H
