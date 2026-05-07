#include "NFCReader.h"
#include "../data/OpenTag3D.h"
#include "../data/OpenPrintTag.h"
#include <vector>

// Hardware pins for CYD SD Card slot
#define SS_PIN 5
#define RST_PIN 22
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23

// Touch CS pin for CYD
#define TOUCH_CS 33

#ifdef USE_PN5180
PN5180ISO15693 NFCReader::pn5180(SS_PIN, CONFIG_NFC_BUSY, RST_PIN);
#else
MFRC522 NFCReader::mfrc522(SS_PIN, RST_PIN);
#endif

void NFCReader::init() {
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, HIGH);

  // Initialize the global SPI hardware bus for the SD slot pins (VSPI)
  // This is safe now because the display is on the HSPI bus.
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1);

#ifdef USE_PN5180
  pn5180.begin();
  
  // Reset with timeout
  digitalWrite(RST_PIN, LOW);
  delay(10);
  digitalWrite(RST_PIN, HIGH);
  delay(10);

  unsigned long start = millis();
  bool success = false;
  while (millis() - start < 500) {
    if (pn5180.getIRQStatus() & 0x01) {
      success = true;
      break;
    }
    delay(10);
  }

  if (!success) {
    Serial.println("NFC: PN5180 hardware not found. Skipping.");
    return;
  }
  
  pn5180.clearIRQStatus(0xffffffff);
  
  uint8_t product[2];
  pn5180.readEEprom(PRODUCT_VERSION, product, 2);
  Serial.print("NFC: PN5180 Product v");
  Serial.print(product[1], HEX);
  Serial.print(".");
  Serial.println(product[0], HEX);

  uint8_t firmware[2];
  pn5180.readEEprom(FIRMWARE_VERSION, firmware, 2);
  Serial.print("NFC: PN5180 Firmware v");
  Serial.print(firmware[1], HEX);
  Serial.print(".");
  Serial.println(firmware[0], HEX);
  
  pn5180.setupRF();
#else
  mfrc522.PCD_Init();

  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (v == 0x00 || v == 0xFF) {
    Serial.println("NFC: MFRC522 communication failure!");
  } else {
    Serial.print("NFC: MFRC522 found (v0x");
    Serial.print(v, HEX);
    Serial.println(")");
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_avg);
  }
#endif
}

bool NFCReader::scanForTag(OpenSpoolData &data) {
#ifdef USE_PN5180
  uint8_t uid[8];
  ISO15693ErrorCode rc = pn5180.getInventory(uid);
  if (rc != ISO15693_EC_OK) return false;

  Serial.println("ISO15693 Tag detected!");
  
  PayloadResult res = readNDEFPayload();
  if (res.type == PayloadType::UNKNOWN) return false;
#else
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  Serial.println("ISO14443A Tag detected!");

  PayloadResult res = readNDEFPayload();
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  if (res.type == PayloadType::UNKNOWN) {
    return false;
  }
#endif

  if (res.type == PayloadType::OPEN_SPOOL_JSON) {
    std::string jsonStr(res.data.begin(), res.data.end());
    return OpenSpoolParser::parseJson(jsonStr, data);
  } else if (res.type == PayloadType::OPEN_TAG_3D_BINARY) {
    return OpenTag3DParser::parseBinary(res.data, data);
  } else if (res.type == PayloadType::OPEN_PRINT_TAG_CBOR) {
    return OpenPrintTagParser::parse(res.data, data);
  }

  return false;
}

bool NFCReader::writeTag(const OpenSpoolData &data) {
#ifdef USE_PN5180
  uint8_t uid[8];
  if (pn5180.getInventory(uid) != ISO15693_EC_OK) return false;
#else
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return false;
  }
#endif

  bool success = false;
  if (data.protocol == "opentag3d") {
    std::vector<uint8_t> payload = OpenTag3DParser::generateBinary(data);
    success = writeNDEFPayload("application/opentag3d", payload);
  } else if (data.protocol == "openprinttag") {
    Serial.println("Writing OpenPrintTag (application/vnd.openprinttag)");
    std::vector<uint8_t> payload = OpenPrintTagParser::generate(data);
    success = writeNDEFPayload("application/vnd.openprinttag", payload);
  } else {
    // Default to OpenSpool JSON
    std::string json = OpenSpoolParser::toJson(data);
    std::vector<uint8_t> payload(json.begin(), json.end());
    success = writeNDEFPayload("application/json", payload);
  }

#ifndef USE_PN5180
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
#endif
  return success;
}

PayloadResult NFCReader::readNDEFPayload() {
  PayloadResult result;
  std::vector<uint8_t> rawData;
  byte buffer[18];

#ifdef USE_PN5180
  // PN5180 (ISO15693) reading logic
  uint8_t uid[8];
  if (pn5180.getInventory(uid) != ISO15693_EC_OK) return result;

  // Read first 32 blocks (usually 4 bytes each)
  for (uint8_t block = 0; block < 32; block++) {
    uint8_t blockData[8];
    // ISO15693 blockSize is usually 4 for ICODE
    if (pn5180.readSingleBlock(uid, block, blockData, 4) == ISO15693_EC_OK) {
      for (uint8_t i = 0; i < 4; i++) rawData.push_back(blockData[i]);
    } else break;
  }
#else
  // MFRC522 (ISO14443A) reading logic
  for (byte page = 4; page < 40; page += 4) {
    byte size = sizeof(buffer);
    if (mfrc522.MIFARE_Read(page, buffer, &size) == MFRC522::STATUS_OK) {
      for (int i = 0; i < 16; i++) rawData.push_back(buffer[i]);
    } else break;
  }
#endif

  if (rawData.empty()) return result;

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
    // Fallback: Check for raw JSON if no NDEF TLV is found
    std::string fallback(rawData.begin(), rawData.end());
    size_t start = fallback.find("{");
    size_t end = fallback.rfind("}");
    if (start != std::string::npos && end != std::string::npos && end > start) {
      result.type = PayloadType::OPEN_SPOOL_JSON;
      std::string json = fallback.substr(start, end - start + 1);
      result.data.assign(json.begin(), json.end());
    }
    return result;
  }

  // Parse NDEF TLV Length
  ndefIdx++; 
  if (ndefIdx >= rawData.size()) return result;

  uint16_t ndefLen = rawData[ndefIdx];
  if (ndefLen == 0xFF) {
    if (ndefIdx + 2 >= rawData.size()) return result;
    ndefLen = (rawData[ndefIdx + 1] << 8) | rawData[ndefIdx + 2];
    ndefIdx += 3;
  } else {
    ndefIdx += 1;
  }

  // NDEF Record Header
  if (ndefIdx >= rawData.size()) return result;

  uint8_t header = rawData[ndefIdx++];
  bool sr = (header & 0x10) != 0; 
  uint8_t typeLen = rawData[ndefIdx++];

  uint32_t payloadLen = 0;
  if (sr) {
    payloadLen = rawData[ndefIdx++];
  } else {
    payloadLen = (rawData[ndefIdx] << 24) | (rawData[ndefIdx + 1] << 16) | (rawData[ndefIdx + 2] << 8) | rawData[ndefIdx + 3];
    ndefIdx += 4;
  }

  std::string mimeType = "";
  for (uint8_t i = 0; i < typeLen && ndefIdx < rawData.size(); i++) {
    mimeType += (char)rawData[ndefIdx++];
  }

  // Load more if needed
  if (ndefIdx + payloadLen > rawData.size()) {
#ifdef USE_PN5180
    uint8_t uid[8];
    pn5180.getInventory(uid);
    uint8_t startBlock = rawData.size() / 4; 
    uint8_t endBlock = (ndefIdx + payloadLen + 3) / 4;
    if (endBlock > 64) endBlock = 64;

    for (uint8_t block = startBlock; block < endBlock; block++) {
      uint8_t blockData[8];
      if (pn5180.readSingleBlock(uid, block, blockData, 4) == ISO15693_EC_OK) {
        for (uint8_t i = 0; i < 4; i++) rawData.push_back(blockData[i]);
      } else break;
    }
#else
    byte lastPage = 4 + (rawData.size() / 4);
    byte endPage = 4 + ((ndefIdx + payloadLen + 3) / 4);
    if (endPage > 128) endPage = 128; 

    for (byte page = lastPage; page < endPage; page += 4) {
      byte size = sizeof(buffer);
      if (mfrc522.MIFARE_Read(page, buffer, &size) == MFRC522::STATUS_OK) {
        for (int i = 0; i < 16; i++) rawData.push_back(buffer[i]);
      } else break;
    }
#endif
  }

  if (ndefIdx + payloadLen <= rawData.size()) {
    result.data.assign(rawData.begin() + ndefIdx, rawData.begin() + ndefIdx + payloadLen);
    if (mimeType == "application/json") result.type = PayloadType::OPEN_SPOOL_JSON;
    else if (mimeType == "application/opentag3d") result.type = PayloadType::OPEN_TAG_3D_BINARY;
    else if (mimeType == "application/openprinttag" || 
             mimeType == "application/vnd.openprinttag" || 
             mimeType == "application/vnd.snapmaker.filament") 
      result.type = PayloadType::OPEN_PRINT_TAG_CBOR;
  }

  return result;
}

bool NFCReader::writeNDEFPayload(const std::string &mimeType, const std::vector<uint8_t> &payload) {
#ifdef USE_PN5180
  uint8_t uid[8];
  if (pn5180.getInventory(uid) != ISO15693_EC_OK) return false;

  size_t payloadLen = payload.size();
  // VCOPT uses a specific 8-byte header before the records:
  // [0..3] Magic: E1 40 27 01
  // [4] NDEF Tag: 0x03
  // [5..7] NDEF Length (3-byte format)
  size_t recordSize = 6 + mimeType.length() + payloadLen; // 6 = header(1) + typeLen(1) + payloadLen(4)
  size_t ndefLen = recordSize;

  std::vector<byte> ndef;
  // Magic Header (Page 4)
  ndef.push_back(0xE1); ndef.push_back(0x40); ndef.push_back(0x27); ndef.push_back(0x01);
  
  // NDEF TLV (Page 5)
  ndef.push_back(0x03); 
  ndef.push_back(0xFF);
  ndef.push_back((byte)((ndefLen >> 8) & 0xFF));
  ndef.push_back((byte)(ndefLen & 0xFF));

  // Records start (Page 6)
  ndef.push_back(0xC2); // MB=1, ME=1, TNF=2 (MIME), SR=0 (Long Record)
  ndef.push_back((byte)mimeType.length());
  ndef.push_back((byte)((payloadLen >> 24) & 0xFF));
  ndef.push_back((byte)((payloadLen >> 16) & 0xFF));
  ndef.push_back((byte)((payloadLen >> 8) & 0xFF));
  ndef.push_back((byte)(payloadLen & 0xFF));
  
  for (char c : mimeType) ndef.push_back((byte)c);
  for (byte b : payload) ndef.push_back(b);
  ndef.push_back(0xFE); // Terminator

  while (ndef.size() % 4 != 0) ndef.push_back(0x00);

  for (size_t i = 0; i < ndef.size(); i += 4) {
    uint8_t block = i / 4;
    // We write starting from block 4 to match VCOPT's user data start
    if (pn5180.writeSingleBlock(uid, 4 + block, &ndef[i], 4) != ISO15693_EC_OK) return false;
  }
  return true;
#else
  // CC for NTAG215 (Standard Page 3)
  byte cc[4] = {0xE1, 0x11, 0x40, 0x00}; 
  if (mfrc522.MIFARE_Ultralight_Write(3, cc, 4) != MFRC522::STATUS_OK) return false;

  size_t payloadLen = payload.size();
  size_t recordSize = 6 + mimeType.length() + payloadLen;
  size_t ndefLen = recordSize; 

  std::vector<byte> ndef;
  // Page 4: VCOPT Magic
  ndef.push_back(0xE1); ndef.push_back(0x40); ndef.push_back(0x27); ndef.push_back(0x01);
  
  // Page 5: NDEF TLV
  ndef.push_back(0x03); 
  ndef.push_back(0xFF);
  ndef.push_back((byte)((ndefLen >> 8) & 0xFF));
  ndef.push_back((byte)(ndefLen & 0xFF));

  // Page 6: Records Start
  ndef.push_back(0xC2); // MB=1, ME=1, TNF=2, SR=0
  ndef.push_back((byte)mimeType.length());
  ndef.push_back((byte)((payloadLen >> 24) & 0xFF));
  ndef.push_back((byte)((payloadLen >> 16) & 0xFF));
  ndef.push_back((byte)((payloadLen >> 8) & 0xFF));
  ndef.push_back((byte)(payloadLen & 0xFF));
  
  for (char c : mimeType) ndef.push_back((byte)c);
  for (byte b : payload) ndef.push_back(b);
  ndef.push_back(0xFE);

  while (ndef.size() % 4 != 0) ndef.push_back(0x00);

  for (size_t i = 0; i < ndef.size(); i += 4) {
    byte page = 4 + (i / 4);
    if (page >= 130) break;
    if (mfrc522.MIFARE_Ultralight_Write(page, &ndef[i], 4) != MFRC522::STATUS_OK) return false;
  }
  return true;
#endif
}
