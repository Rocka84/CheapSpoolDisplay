#include "NFCReader.h"
#include "../data/OpenTag3D.h"
#include <vector>

// Hardware pins for CYD SD Card slot
#define SS_PIN 5
#define RST_PIN 22
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23

// Touch CS pin for CYD
#define TOUCH_CS 33

MFRC522 NFCReader::mfrc522(SS_PIN, RST_PIN);

void NFCReader::init() {
  // Initialize the global SPI hardware bus for the SD slot pins
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, 255);

  // Explicitly initialize the MFRC522 CS pin
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);

  mfrc522.PCD_Init();

  // Diagnostic check
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print("MFRC522 Firmware Version: 0x");
  Serial.println(v, HEX);
  if (v == 0x00 || v == 0xFF) {
    Serial.println(
        "WARNING: Communication failure, is the MFRC522 properly connected?");
  } else {
    // Increase antenna gain to moderate level (38dB) to improve range without
    // saturation
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_avg);
  }
}

bool NFCReader::scanForTag(OpenSpoolData &data) {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  Serial.println("Tag detected!");

  PayloadResult res = readNDEFPayload();
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  if (res.type == PayloadType::UNKNOWN) {
    Serial.println("Error: Unknown or empty NDEF payload.");
    return false;
  }

  if (res.type == PayloadType::OPEN_SPOOL_JSON) {
    std::string jsonStr(res.data.begin(), res.data.end());
    bool parsed = OpenSpoolParser::parseJson(jsonStr, data);
    if (!parsed) {
      Serial.printf("JSON parse failed. Raw payload:\n%s\n", jsonStr.c_str());
    }
    return parsed;
  } else if (res.type == PayloadType::OPEN_TAG_3D_BINARY) {
    bool parsed = OpenTag3DParser::parseBinary(res.data, data);
    if (!parsed) {
      Serial.println("OpenTag3D binary parse failed.");
    }
    return parsed;
  }

  return false;
}

bool NFCReader::writeTag(const OpenSpoolData &data) {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  bool success = false;
  if (data.protocol == "opentag3d") {
    std::vector<uint8_t> payload = OpenTag3DParser::generateBinary(data);
    success = writeNDEFPayload("application/opentag3d", payload);
  } else {
    // Default to OpenSpool JSON
    std::string json = OpenSpoolParser::toJson(data);
    std::vector<uint8_t> payload(json.begin(), json.end());
    success = writeNDEFPayload("application/json", payload);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return success;
}

PayloadResult NFCReader::readNDEFPayload() {
  PayloadResult result;
  std::vector<uint8_t> rawData;
  byte buffer[18];

  // Read first few pages to find NDEF TLV
  // Page 4 starts user data
  for (byte page = 4; page < 40; page += 4) {
    byte size = sizeof(buffer);
    MFRC522::StatusCode status = mfrc522.MIFARE_Read(page, buffer, &size);
    if (status == MFRC522::STATUS_OK) {
      for (int i = 0; i < 16; i++) {
        rawData.push_back(buffer[i]);
      }
    } else {
      break;
    }
  }

  if (rawData.empty())
    return result;

  // Find NDEF TLV (0x03)
  size_t ndefIdx = 0;
  bool foundTlv = false;
  for (size_t i = 0; i < rawData.size(); i++) {
    if (rawData[i] == 0x03) {
      ndefIdx = i;
      foundTlv = true;
      break;
    }
  }

  if (!foundTlv) {
    // Fallback: Check for raw JSON if no NDEF TLV is found (legacy/non-standard)
    std::string fallback(rawData.begin(), rawData.end());
    size_t start = fallback.find("{");
    size_t end = fallback.rfind("}");
    if (start != std::string::npos && end != std::string::npos && end > start) {
      result.type = PayloadType::OPEN_SPOOL_JSON;
      std::string json = fallback.substr(start, end - start + 1);
      result.data.assign(json.begin(), json.end());
      return result;
    }
    return result;
  }

  // Parse NDEF TLV Length
  ndefIdx++; // Move to length
  if (ndefIdx >= rawData.size())
    return result;

  uint16_t ndefLen = rawData[ndefIdx];
  if (ndefLen == 0xFF) {
    if (ndefIdx + 2 >= rawData.size())
      return result;
    ndefLen = (rawData[ndefIdx + 1] << 8) | rawData[ndefIdx + 2];
    ndefIdx += 3;
  } else {
    ndefIdx += 1;
  }

  // Now at NDEF Record Header
  if (ndefIdx >= rawData.size())
    return result;

  uint8_t header = rawData[ndefIdx++];
  bool sr = (header & 0x10) != 0; // Short Record
  uint8_t typeLen = rawData[ndefIdx++];

  uint32_t payloadLen = 0;
  if (sr) {
    payloadLen = rawData[ndefIdx++];
  } else {
    payloadLen = (rawData[ndefIdx] << 24) | (rawData[ndefIdx + 1] << 16) |
                 (rawData[ndefIdx + 2] << 8) | rawData[ndefIdx + 3];
    ndefIdx += 4;
  }

  // Record Type
  std::string mimeType = "";
  for (uint8_t i = 0; i < typeLen && ndefIdx < rawData.size(); i++) {
    mimeType += (char)rawData[ndefIdx++];
  }

  // Extract Payload
  // If payloadLen exceeds current rawData, we need to read more from the tag
  if (ndefIdx + payloadLen > rawData.size()) {
    // Read remaining pages
    byte lastPage = 4 + (rawData.size() / 4);
    byte endPage = 4 + ((ndefIdx + payloadLen + 3) / 4);
    if (endPage > 128)
      endPage = 128; // Safety limit

    for (byte page = lastPage; page < endPage; page += 4) {
      byte size = sizeof(buffer);
      MFRC522::StatusCode status = mfrc522.MIFARE_Read(page, buffer, &size);
      if (status == MFRC522::STATUS_OK) {
        for (int i = 0; i < 16; i++) {
          rawData.push_back(buffer[i]);
        }
      } else {
        break;
      }
    }
  }

  if (ndefIdx + payloadLen <= rawData.size()) {
    result.data.assign(rawData.begin() + ndefIdx,
                       rawData.begin() + ndefIdx + payloadLen);

    if (mimeType == "application/json") {
      result.type = PayloadType::OPEN_SPOOL_JSON;
    } else if (mimeType == "application/opentag3d") {
      result.type = PayloadType::OPEN_TAG_3D_BINARY;
    }
  }

  return result;
}

bool NFCReader::writeNDEFPayload(const std::string &mimeType, const std::vector<uint8_t> &payload) {
  // NDEF formatting for NTAG215
  // Page 3: CC (Capability Container) - E1 10 3E 00 for NTAG215 (504 bytes)
  byte cc[4] = {0xE1, 0x10, 0x3E, 0x00};
  MFRC522::StatusCode status = mfrc522.MIFARE_Ultralight_Write(3, cc, 4);
  if (status != MFRC522::STATUS_OK) {
    Serial.printf("CC Write failed: %s\n", mfrc522.GetStatusCodeName(status));
    return false;
  }

  // Build NDEF Message
  // TNF 0x02 (Mime Media)
  size_t payloadLen = payload.size();
  size_t ndefLen =
      3 + mimeType.length() +
      payloadLen; // Header + TypeLength + PayloadLength + Type + Payload

  std::vector<byte> ndef;
  ndef.push_back(0x03); // NDEF Message TLV
  if (ndefLen < 255) {
    ndef.push_back((byte)ndefLen);
  } else {
    ndef.push_back(0xFF);
    ndef.push_back((byte)((ndefLen >> 8) & 0xFF));
    ndef.push_back((byte)(ndefLen & 0xFF));
  }

  // NDEF Record Header
  // MB=1, ME=1, CF=0, SR=1, IL=0, TNF=0x02
  ndef.push_back(0xD2);
  ndef.push_back((byte)mimeType.length());
  ndef.push_back((byte)payloadLen);
  for (char c : mimeType)
    ndef.push_back((byte)c);
  for (byte b : payload)
    ndef.push_back(b);
  ndef.push_back(0xFE); // Terminator TLV

  // Pad to 4-byte pages
  while (ndef.size() % 4 != 0) {
    ndef.push_back(0x00);
  }

  // Write pages starting at 4
  for (size_t i = 0; i < ndef.size(); i += 4) {
    byte page = 4 + (i / 4);
    if (page >= 130)
      break; // NTAG215 limit
    status = mfrc522.MIFARE_Ultralight_Write(page, &ndef[i], 4);
    if (status != MFRC522::STATUS_OK) {
      Serial.printf("Write failed at page %d: %s\n", page,
                    mfrc522.GetStatusCodeName(status));
      return false;
    }
  }

  return true;
}
