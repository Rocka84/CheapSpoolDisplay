#ifndef NFC_READER_H
#define NFC_READER_H

#include "../data/OpenSpool.h"
#ifdef USE_PN5180
// Hack to bypass library access controls without modifying external code
#define private public
#include <PN5180ISO15693.h>
#include <PN5180ISO14443.h>
#undef private
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
  static PN5180ISO15693 pn5180_15693;
  static PN5180ISO14443 pn5180_14443;
  
  // Custom Crypto1 Auth function using the unlocked PN5180 library
  static bool pn5180_mifareAuthenticate(uint8_t *key, uint8_t keyType, uint8_t blockAddress, uint8_t *uid);
#else
  static MFRC522 mfrc522;
#endif

  // Protocol-specific readers
  static PayloadResult readNDEFPayload(bool is15693 = true);
  static bool readSnapmakerTag(OpenSpoolData &data, uint8_t *uid = nullptr, uint8_t uidLen = 0, bool *isSnapmaker = nullptr);
  static int readBambuTag(OpenSpoolData &data, uint8_t *uid = nullptr, uint8_t uidLen = 0);
  static bool writeNDEFPayload(const std::string &mimeType, const std::vector<uint8_t> &payload, bool is15693 = true);
};

#endif // NFC_READER_H
