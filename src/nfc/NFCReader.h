#ifndef NFC_READER_H
#define NFC_READER_H

#include "../data/OpenSpool.h"
#include <MFRC522.h>
#include <SPI.h>

class NFCReader {
public:
  static void init();

  // Returns true if a tag was read successfully and parsed into `data`
  static bool scanForTag(OpenSpoolData &data);

  // Returns true if write was successful
  static bool writeTag(const OpenSpoolData &data);

private:
  static MFRC522 mfrc522;

  // NDEF reading helpers (simplified for OpenSpool JSON payload on NTAG215)
  static std::string readNDEFPayload();
  static bool writeNDEFPayload(const std::string &json);
};

#endif // NFC_READER_H
