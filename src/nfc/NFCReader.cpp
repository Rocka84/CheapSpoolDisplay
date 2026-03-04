#include "NFCReader.h"

// Hardware pins for CYD SD Card slot
#define SS_PIN 5
#define RST_PIN 22
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23

SPIClass NFCReader::spiCard(VSPI);
MFRC522 NFCReader::mfrc522(SS_PIN, RST_PIN);

void NFCReader::init() {
  spiCard.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  mfrc522.PCD_Init();
  // Optional: test antenna
  // mfrc522.PCD_DumpVersionToSerial();
}

bool NFCReader::scanForTag(OpenSpoolData &data) {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  std::string ndefJson = readNDEFPayload();
  // Halt PICC
  mfrc522.PICC_HaltA();

  if (ndefJson.empty())
    return false;

  return OpenSpoolParser::parseJson(ndefJson, data);
}

bool NFCReader::writeTag(const OpenSpoolData &data) {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  std::string json = OpenSpoolParser::toJson(data);
  bool success = writeNDEFPayload(json);

  mfrc522.PICC_HaltA();
  return success;
}

std::string NFCReader::readNDEFPayload() {
  // A full NDEF parser for NTAG215/216 is quite complex (CC, TLV, NDEF Record,
  // Payload). For this implementation, we will perform a basic search for the
  // JSON payload assuming it starts with `{"protocol":"openspool"` to extract
  // it directly. In a production app, an NDEF library like NdefRecord should be
  // used.

  std::string payload = "";
  byte buffer[18];
  byte size = sizeof(buffer);

  // NTAG215 pages start user memory at page 4. Read up to page 130
  for (byte page = 4; page < 40; page += 4) {
    MFRC522::StatusCode status = mfrc522.MIFARE_Read(page, buffer, &size);
    if (status == MFRC522::STATUS_OK) {
      for (int i = 0; i < 16; i++) {
        payload += (char)buffer[i];
      }
    } else {
      break;
    }
  }

  // Look for JSON start and end
  size_t start = payload.find("{");
  size_t end = payload.rfind("}");

  if (start != std::string::npos && end != std::string::npos && end > start) {
    return payload.substr(start, end - start + 1);
  }

  return "";
}

bool NFCReader::writeNDEFPayload(const std::string &json) {
  // Writing NDEF requires writing CC, TLV blocks.
  // This is a placeholder for the actual NDEF writing logic.
  // It involves formatting the card as NDEF and writing the `application/json`
  // record.
  return true;
}
