#ifndef NFC_READER_H
#define NFC_READER_H

#include "../data/OpenSpool.h"
#ifdef USE_PN5180
#include <PN5180ISO15693.h>
#else
#include <MFRC522.h>
#endif
#include <SPI.h>

enum class PayloadType {
  UNKNOWN,
  OPEN_SPOOL_JSON,
  OPEN_TAG_3D_BINARY,
  OPEN_PRINT_TAG_CBOR
};

struct PayloadResult {
  PayloadType type = PayloadType::UNKNOWN;
  std::vector<uint8_t> data;
};

class NFCReader {
public:
  static void init();

  // Returns true if a tag was read successfully and parsed into `data`
  static bool scanForTag(OpenSpoolData &data);

  // Returns true if write was successful
  static bool writeTag(const OpenSpoolData &data);

private:
#ifdef USE_PN5180
  static PN5180ISO15693 pn5180;
#else
  static MFRC522 mfrc522;
#endif

  // Protocol-specific readers
  static PayloadResult readNDEFPayload();
  static bool readSnapmakerTag(OpenSpoolData &data);
  static bool readBambuTag(OpenSpoolData &data);
  static bool writeNDEFPayload(const std::string &mimeType, const std::vector<uint8_t> &payload);
};

#endif // NFC_READER_H
