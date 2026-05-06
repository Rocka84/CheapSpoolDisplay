#ifndef NFC_READER_H
#define NFC_READER_H

#include "../data/OpenSpool.h"
#include <MFRC522.h>
#include <SPI.h>

enum class PayloadType {
  UNKNOWN,
  OPEN_SPOOL_JSON,
  OPEN_TAG_3D_BINARY
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
  static MFRC522 mfrc522;

  // NDEF reading helpers (simplified for multi-protocol support)
  static PayloadResult readNDEFPayload();
  static bool writeNDEFPayload(const std::string &mimeType, const std::vector<uint8_t> &payload);
};

#endif // NFC_READER_H
