#ifndef OPENSPOOL_H
#define OPENSPOOL_H
#ifdef USE_SDL2
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_PROGMEM 0
#endif
#include <ArduinoJson.h>
#include <string>
#include <vector>

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
  std::string alpha;
  std::string diameter;
  std::string hardware_uid; // Hardware serial/UID for read-only tags

  // Technical & Manufacturing fields
  std::string actual_weight;
  std::string empty_weight;
  std::string density;
  std::string dry_temp;
  std::string dry_time;
  std::string td;
  std::string shore;
  std::string tags;
  std::string gtin;

  // Inventory & Spoolman fields
  std::string filament_name;
  std::string remaining_weight;
  std::string total_weight;
  std::string location;
  std::string price;
  std::string notes;
  std::string first_used;
  std::string last_used;

  OpenSpoolData() { reset(); }

  void reset() {
    protocol = "openspool";
    version = "1";
    type = "";
    color_hex = "";
    brand = "";
    spool_id = "";
    min_temp = "";
    max_temp = "";
    bed_min_temp = "";
    bed_max_temp = "";
    lot_nr = "";
    subtype = "";
    alpha = "";
    filament_name = "";
    remaining_weight = "";
    total_weight = "";
    diameter = "";
    actual_weight = "";
    empty_weight = "";
    density = "";
    dry_temp = "";
    dry_time = "";
    td = "";
    shore = "";
    tags = "";
    gtin = "";
    location = "";
    price = "";
    notes = "";
    first_used = "";
    last_used = "";
    hardware_uid = "";
  }
};

struct SpoolmanItem {
  std::string id;
  std::string brand;
  std::string type;
  std::string name;
  std::string color_hex;
  float remaining_weight;
};

class OpenSpoolParser {
public:
  static bool parseJson(const std::string &json, OpenSpoolData &data);
  static bool enrichFromSpoolman(const std::string &json, OpenSpoolData &data);
  static bool parseSpoolmanList(const std::string &json, std::vector<SpoolmanItem> &items, int &total_count);
  static std::string toJson(const OpenSpoolData &data);
};

#endif // OPENSPOOL_H
