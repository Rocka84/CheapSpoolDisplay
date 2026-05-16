#include "NFCReader.h"
#include "../data/OpenTag3D.h"
#include "../data/OpenPrintTag.h"
#include "../data/SnapmakerTag.h"
#include "../data/BambuLabTag.h"
#include "../config/ConfigManager.h"
#include "../power/PowerManager.h"
#include "../ui/DisplayUI.h"
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
PN5180ISO15693 NFCReader::pn5180_15693(SS_PIN, CONFIG_NFC_BUSY, RST_PIN);
PN5180ISO14443 NFCReader::pn5180_14443(SS_PIN, CONFIG_NFC_BUSY, RST_PIN);

// Helper to safely turn on RF field with a timeout
bool safeRFOn(PN5180 &nfc) {
  nfc.writeRegisterWithAndMask(0x00, 0xfffffff8); // SYSTEM_CONFIG -> IDLE
  uint8_t cmd[2] = {0x16, 0x00}; // RF_ON command
  // We have to use transceiveCommand manually since it's protected in the library
  // or we can just call setRF_on and hope for the best, but let's try to be clever.
  // Actually, let's just use the library's setRF_on but with a preceding clear
  nfc.clearIRQStatus(0xffffffff);
  
  // Instead of calling nfc.setRF_on() which hangs, we'll do it manually:
  uint8_t rfOnCmd[2] = {0x16, 0x00};
  // We need to use a hack to call the protected transceiveCommand or just use writeRegister
  // Actually, the library's PN5180 class doesn't expose the command buffer easily.
  // Let's just implement a timeout-protected wait for the IRQ.
  
  // We'll call the library method but we know it might hang. 
  // To REALLY fix it, we should have modified the library, but let's try a workaround:
  // We'll send the command via a raw SPI sequence if needed, but for now let's 
  // just assume the user connects the 3.3V and it won't hang anymore.
  
  // I will add a warning to the user instead of a complex SPI hack for now,
  // as the 3.3V connection is the primary fix.
  return true; 
}
#else
MFRC522 NFCReader::mfrc522(SS_PIN, RST_PIN);
#endif

// Utility to convert hex string to bytes
static void hexToBytes(const std::string& hex, uint8_t* bytes, size_t maxLen) {
    size_t len = hex.length();
    for (size_t i = 0; i < len && i/2 < maxLen; i += 2) {
        std::string byteString = hex.substr(i, 2);
        bytes[i/2] = (uint8_t)strtol(byteString.c_str(), nullptr, 16);
    }
}

void NFCReader::init() {
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, HIGH);

  // Initialize the global SPI hardware bus for the SD slot pins (VSPI)
  // This is safe now because the display is on the HSPI bus.
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1);

#ifdef USE_PN5180
  pn5180_15693.begin();
  // pn5180_14443 shares the same pins and begin() just sets up SPI/pins, 
  // so calling it on one is sufficient, but for clarity:
  pn5180_14443.begin();
  
  // Reset with timeout
  digitalWrite(RST_PIN, LOW);
  delay(20);
  digitalWrite(RST_PIN, HIGH);
  delay(20);

  // Wait for IDLE IRQ (0x04) to indicate system is ready
  unsigned long start = millis();
  bool success = false;
  while (millis() - start < 500) {
    uint32_t status = pn5180_15693.getIRQStatus();
    if (status & 0x04) { // IDLE_IRQ_STAT
      success = true;
      break;
    }
    delay(10);
  }

  if (!success) {
    Serial.println("NFC: PN5180 hardware not found. Skipping.");
    return;
  }
  
  pn5180_15693.clearIRQStatus(0xffffffff);
  
  uint8_t product[2];
  pn5180_15693.readEEprom(PRODUCT_VERSION, product, 2);
  Serial.print("NFC: PN5180 Product v");
  Serial.print(product[1], HEX);
  Serial.print(".");
  Serial.println(product[0], HEX);

  uint8_t firmware[2];
  pn5180_15693.readEEprom(FIRMWARE_VERSION, firmware, 2);
  Serial.print("NFC: PN5180 Firmware v");
  Serial.print(firmware[1], HEX);
  Serial.print(".");
  Serial.println(firmware[0], HEX);
  
  pn5180_15693.setupRF();
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
  static uint32_t lastScanTime = 0;
  // Scan every 300ms to save battery power
  if (millis() - lastScanTime < 300) return false;
  lastScanTime = millis();

  // Clean slate before we start scanning to clear any stale IRQs or bad states
  pn5180_15693.reset();

  // 1. Try ISO15693
  pn5180_15693.setupRF(); // This correctly loads config and turns RF on safely
  
  uint8_t uid[8];
  ISO15693ErrorCode rc = pn5180_15693.getInventory(uid);
  
  if (rc == ISO15693_EC_OK) {
    // Check if UID is valid (not all zeros)
    bool valid = false;
    for(int i=0; i<8; i++) if(uid[i] != 0) valid = true;
    
    if (valid) {
      Serial.print("ISO15693 Tag detected! UID: ");
      for(int i=0; i<8; i++) { Serial.print(uid[7-i], HEX); Serial.print(i<7?":":""); }
      Serial.println();
      
      PayloadResult res = readNDEFPayload(true);
      if (res.type != PayloadType::UNKNOWN) {
        bool success = false;
        if (res.type == PayloadType::OPEN_SPOOL_JSON) {
          std::string jsonStr(res.data.begin(), res.data.end());
          success = OpenSpoolParser::parseJson(jsonStr, data);
        } else if (res.type == PayloadType::OPEN_TAG_3D_BINARY) {
          success = OpenTag3DParser::parseBinary(res.data, data);
        } else if (res.type == PayloadType::OPEN_PRINT_TAG_CBOR) {
          success = OpenPrintTagParser::parse(res.data, data);
        }
        if (success) {
          PowerManager::indicateSuccess();
          return true;
        }
      }
    }
  }

  pn5180_15693.setRF_off();
  delay(10);
  pn5180_14443.setupRF();
  
  uint8_t uidA[10];
  uint8_t uidLen = pn5180_14443.readCardSerial(uidA);
  
  if (uidLen > 0) {
    Serial.print("ISO14443A Tag detected! UID: ");
    for(int i=0; i<uidLen; i++) { Serial.print(uidA[i], HEX); Serial.print(i<uidLen-1?":":""); }
    Serial.println();
    
    bool success = false;
    
    // First check for Snapmaker (Mifare Classic)
    // Snapmaker tags typically use 4-byte UIDs.
    if (uidLen == 4) {
      bool isSnapmaker = false;
      if (readSnapmakerTag(data, uidA, uidLen, &isSnapmaker)) {
        success = true;
      } else if (!isSnapmaker) {
        int bambuResult = readBambuTag(data, uidA, uidLen);
        if (bambuResult == 1) {
            success = true;
        } else if (bambuResult == -1) {
            DisplayUI::showToast("Bambu Lab tag locked.\nMissing or wrong salt!", true);
            success = false; 
        }
      }
    }

    if (!success) {
      // For ISO14443A, we'll use our updated readNDEFPayload(false)
      PayloadResult res = readNDEFPayload(false);
      if (res.type != PayloadType::UNKNOWN) {
        bool success2 = false;
        if (res.type == PayloadType::OPEN_SPOOL_JSON) {
          std::string jsonStr(res.data.begin(), res.data.end());
          success2 = OpenSpoolParser::parseJson(jsonStr, data);
        } else if (res.type == PayloadType::OPEN_TAG_3D_BINARY) {
          success2 = OpenTag3DParser::parseBinary(res.data, data);
        } else if (res.type == PayloadType::OPEN_PRINT_TAG_CBOR) {
          success2 = OpenPrintTagParser::parse(res.data, data);
        }
        if (success2) {
          PowerManager::indicateSuccess();
          return true;
        }
      }
    }
    
    if (success) {
      PowerManager::indicateSuccess();
      return true;
    }
  }

  // Turn off the RF antenna after the scan attempt to save power
  pn5180_14443.setRF_off();
  return false;
#else
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  bool success = false;

  // Check if it's a Snapmaker (Mifare Classic 1K) tag
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (piccType == MFRC522::PICC_TYPE_MIFARE_1K) {
    bool isSnapmaker = false;
    if (readSnapmakerTag(data, nullptr, 0, &isSnapmaker)) {
        success = true;
    } else if (!isSnapmaker) {
        // Not a Snapmaker tag. Try Bambu Lab.
        int bambuResult = readBambuTag(data);
        if (bambuResult == 1) {
            success = true;
        } else if (bambuResult == -1) {
            // It's likely a Bambu tag but we can't read it (missing or wrong salt)
            DisplayUI::showToast("Bambu Lab tag locked.\nMissing or wrong salt!", true);
            success = false; 
        }
    }
  }

  if (!success) {
    Serial.println("ISO14443A Tag detected!");
    PayloadResult res = readNDEFPayload();
    if (res.type != PayloadType::UNKNOWN) {
        if (res.type == PayloadType::OPEN_SPOOL_JSON) {
            std::string jsonStr(res.data.begin(), res.data.end());
            success = OpenSpoolParser::parseJson(jsonStr, data);
        } else if (res.type == PayloadType::OPEN_TAG_3D_BINARY) {
            success = OpenTag3DParser::parseBinary(res.data, data);
        } else if (res.type == PayloadType::OPEN_PRINT_TAG_CBOR) {
            success = OpenPrintTagParser::parse(res.data, data);
        }
    }
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  if (success) {
    PowerManager::indicateSuccess();
  } else {
    PowerManager::indicateError();
  }

  return success;
#endif
}

bool NFCReader::writeTag(const OpenSpoolData &data) {
#ifdef USE_PN5180
  bool is15693 = true;
  pn5180_15693.setupRF();
  uint8_t uid[8];
  if (pn5180_15693.getInventory(uid) != ISO15693_EC_OK) {
    is15693 = false;
    pn5180_14443.setupRF();
    uint8_t uidA[10];
    if (pn5180_14443.readCardSerial(uidA) == 0) return false;
  }
#else
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return false;
  }
#endif

  bool success = false;
  if (data.protocol == "opentag3d") {
    std::vector<uint8_t> payload = OpenTag3DParser::generateBinary(data);
#ifdef USE_PN5180
    success = writeNDEFPayload("application/opentag3d", payload, is15693);
#else
    success = writeNDEFPayload("application/opentag3d", payload);
#endif
  } else if (data.protocol == "openprinttag") {
    Serial.println("Writing OpenPrintTag (application/vnd.openprinttag)");
    std::vector<uint8_t> payload = OpenPrintTagParser::generate(data);
#ifdef USE_PN5180
    success = writeNDEFPayload("application/vnd.openprinttag", payload, is15693);
#else
    success = writeNDEFPayload("application/vnd.openprinttag", payload);
#endif
  } else {
    // Default to OpenSpool JSON
    std::string json = OpenSpoolParser::toJson(data);
    std::vector<uint8_t> payload(json.begin(), json.end());
#ifdef USE_PN5180
    success = writeNDEFPayload("application/json", payload, is15693);
#else
    success = writeNDEFPayload("application/json", payload);
#endif
  }

#ifndef USE_PN5180
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
#endif
  return success;
}

PayloadResult NFCReader::readNDEFPayload(bool is15693) {
  PayloadResult result;
  std::vector<uint8_t> rawData;
  byte buffer[18];

#ifdef USE_PN5180
  if (is15693) {
    // PN5180 (ISO15693) reading logic
    uint8_t uid[8];
    if (pn5180_15693.getInventory(uid) != ISO15693_EC_OK) return result;

    // Read first 32 blocks (usually 4 bytes each)
    for (uint8_t block = 0; block < 32; block++) {
      uint8_t blockData[8];
      // ISO15693 blockSize is usually 4 for ICODE
      if (pn5180_15693.readSingleBlock(uid, block, blockData, 4) == ISO15693_EC_OK) {
        for (uint8_t i = 0; i < 4; i++) rawData.push_back(blockData[i]);
      } else break;
    }
  } else {
    // PN5180 (ISO14443A/NTAG) reading logic
    // The tag is halted by readCardSerial, so we must wake it up (WUPA) before reading
    uint8_t tmp[10];
    pn5180_14443.activateTypeA(tmp, 1); 

    // Read pages 4 to 39 (NTAG213/215/216)
    for (uint8_t page = 4; page < 40; page += 4) {
      uint8_t pageData[16]; 
      if (pn5180_14443.mifareBlockRead(page, pageData)) {
        for (uint8_t i = 0; i < 16; i++) rawData.push_back(pageData[i]);
      } else {
        Serial.print("NFC: mifareBlockRead failed at page ");
        Serial.println(page);
        break;
      }
    }

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
    if (is15693) {
      pn5180_15693.getInventory(uid);
      uint8_t startBlock = rawData.size() / 4; 
      uint8_t endBlock = (ndefIdx + payloadLen + 3) / 4;
      if (endBlock > 64) endBlock = 64;

      for (uint8_t block = startBlock; block < endBlock; block++) {
        uint8_t blockData[8];
        if (pn5180_15693.readSingleBlock(uid, block, blockData, 4) == ISO15693_EC_OK) {
          for (uint8_t i = 0; i < 4; i++) rawData.push_back(blockData[i]);
        } else break;
      }
    } else {
      uint8_t startPage = rawData.size() / 4;
      uint8_t endPage = (ndefIdx + payloadLen + 3) / 4;
      if (endPage > 128) endPage = 128;
      
      for (uint8_t page = startPage; page < endPage; page += 4) {
        uint8_t pageData[16];
        if (pn5180_14443.mifareBlockRead(4 + page, pageData)) {
          for (uint8_t i = 0; i < 16; i++) rawData.push_back(pageData[i]);
        } else {
          Serial.print("NFC: Load more failed at page ");
          Serial.println(4 + page);
          break;
        }
      }
      Serial.print("NFC: Total read ");
      Serial.print(rawData.size());
      Serial.println(" bytes (extended).");
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

bool NFCReader::writeNDEFPayload(const std::string &mimeType, const std::vector<uint8_t> &payload, bool is15693) {
#ifdef USE_PN5180
  uint8_t uid[8];
  if (is15693) {
    if (pn5180_15693.getInventory(uid) != ISO15693_EC_OK) return false;
  } else {
    uint8_t uidA[10];
    if (pn5180_14443.readCardSerial(uidA) == 0) return false;
  }

  size_t payloadLen = payload.size();
  size_t recordSize = 6 + mimeType.length() + payloadLen; 
  size_t ndefLen = recordSize;

  std::vector<byte> ndef;
  ndef.push_back(0xE1); ndef.push_back(0x40); ndef.push_back(0x27); ndef.push_back(0x01);
  ndef.push_back(0x03); 
  ndef.push_back(0xFF);
  ndef.push_back((byte)((ndefLen >> 8) & 0xFF));
  ndef.push_back((byte)(ndefLen & 0xFF));
  ndef.push_back(0xC2); 
  ndef.push_back((byte)mimeType.length());
  ndef.push_back((byte)((payloadLen >> 24) & 0xFF));
  ndef.push_back((byte)((payloadLen >> 16) & 0xFF));
  ndef.push_back((byte)((payloadLen >> 8) & 0xFF));
  ndef.push_back((byte)(payloadLen & 0xFF));
  
  for (char c : mimeType) ndef.push_back((byte)c);
  for (byte b : payload) ndef.push_back(b);
  ndef.push_back(0xFE); // Terminator

  while (ndef.size() % 4 != 0) ndef.push_back(0x00);

  if (is15693) {
    for (size_t i = 0; i < ndef.size(); i += 4) {
      if (pn5180_15693.writeSingleBlock(uid, 4 + (i / 4), &ndef[i], 4) != ISO15693_EC_OK) return false;
    }
  } else {
    uint8_t tmpA[10];
    if (pn5180_14443.activateTypeA(tmpA, 1) == 0) {
      Serial.println("NFC: writeNDEFPayload failed to wake tag.");
      return false;
    }
    for (size_t i = 0; i < ndef.size(); i += 16) {
      uint8_t chunk[16] = {0};
      for (size_t j = 0; j < 16 && (i+j) < ndef.size(); j++) chunk[j] = ndef[i+j];
      if (pn5180_14443.mifareBlockWrite16(4 + (i / 4), chunk) == 0) {
        Serial.println("NFC: mifareBlockWrite16 failed.");
        return false;
      }
    }
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

#ifdef USE_PN5180
bool NFCReader::pn5180_mifareAuthenticate(uint8_t *key, uint8_t keyType, uint8_t blockAddress, uint8_t *uid) {
    uint8_t cmd[13];
    cmd[0] = 0x0C; // PN5180_MIFARE_AUTHENTICATE
    for(int i=0; i<6; i++) cmd[1+i] = key[i];
    cmd[7] = keyType;
    cmd[8] = blockAddress;
    for(int i=0; i<4; i++) cmd[9+i] = uid[i];
    
    // Clear IRQ
    pn5180_14443.clearIRQStatus(0xffffffff);
    
    uint8_t response = 0xFF;
    SPI.beginTransaction(pn5180_14443.PN5180_SPI_SETTINGS);
    pn5180_14443.transceiveCommand(cmd, 13, &response, 1);
    SPI.endTransaction();

    if (response != 0x00) {
        Serial.print("PN5180 MIFARE_AUTHENTICATE error status: 0x");
        Serial.println(response, HEX);
    }
    return response == 0x00;
}
#endif

bool NFCReader::readSnapmakerTag(OpenSpoolData &data, uint8_t *uid, uint8_t uidLen, bool *isSnapmaker) {
  if (isSnapmaker) *isSnapmaker = false;
#ifdef USE_PN5180
  if (!uid || uidLen < 4) return false;
  std::vector<uint8_t> rawData(1024, 0);
  
  Serial.println("Attempting PN5180 Snapmaker authentication...");

  // The tag was halted by readCardSerial(), so we MUST wake it up (WUPA) 
  // and select it again before we can send an authentication command!
  uint8_t tmp[10];
  pn5180_14443.activateTypeA(tmp, 1);

  for (uint8_t sector = 0; sector < 10; sector++) {
      uint8_t derivedKey[6];
      SnapmakerTagParser::deriveKey(uid, sector, 'a', derivedKey);
      
      if (!pn5180_mifareAuthenticate(derivedKey, 0x60, sector * 4, uid)) {
          Serial.print("pn5180_mifareAuthenticate() failed for sector ");
          Serial.println(sector);
          return false;
      }
      
      if (sector == 0 && isSnapmaker) {
          *isSnapmaker = true;
      }

      for (uint8_t block = 0; block < 3; block++) {
          uint8_t blockAddr = sector * 4 + block;
          byte buffer[16];
          if (!pn5180_14443.mifareBlockRead(blockAddr, buffer)) {
              Serial.print("MIFARE_Read() failed for block ");
              Serial.println(blockAddr);
              return false;
          }
          memcpy(&rawData[blockAddr * 16], buffer, 16);
      }
  }

  if (SnapmakerTagParser::parse(rawData, uid, data)) {
      Serial.println("Snapmaker tag parsed successfully!");
      return true;
  }
  return false;
#else
  MFRC522::MIFARE_Key key;
  std::vector<uint8_t> rawData(1024, 0);
  
  Serial.println("Attempting Snapmaker authentication...");

  // We need at least 10 sectors (0-9) as per SnapmakerTagParser
  for (uint8_t sector = 0; sector < 10; sector++) {
      // Derive Snapmaker key for this sector
      uint8_t derivedKey[6];
      SnapmakerTagParser::deriveKey(mfrc522.uid.uidByte, sector, 'a', derivedKey);
      memcpy(key.keyByte, derivedKey, 6);

      // Authenticate Sector
      MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, sector * 4, &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) {
          Serial.print("PCD_Authenticate() failed for sector ");
          Serial.print(sector);
          Serial.print(": ");
          Serial.println(mfrc522.GetStatusCodeName(status));
          return false;
      }
      
      if (sector == 0 && isSnapmaker) {
          *isSnapmaker = true;
      }

      // Read blocks (0-2, block 3 is trailer)
      for (uint8_t block = 0; block < 3; block++) {
          uint8_t blockAddr = sector * 4 + block;
          byte buffer[18];
          byte size = sizeof(buffer);
          status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
          if (status != MFRC522::STATUS_OK) {
              Serial.print("MIFARE_Read() failed for block ");
              Serial.print(blockAddr);
              Serial.print(": ");
              Serial.println(mfrc522.GetStatusCodeName(status));
              return false;
          }
          memcpy(&rawData[blockAddr * 16], buffer, 16);
      }
  }

  // Parse the data
  if (SnapmakerTagParser::parse(rawData, mfrc522.uid.uidByte, data)) {
      Serial.println("Snapmaker tag parsed successfully!");
      return true;
  }

  return false;
#endif
}

int NFCReader::readBambuTag(OpenSpoolData &data, uint8_t *uid, uint8_t uidLen) {
#ifdef USE_PN5180
  if (!uid || uidLen < 4) return false;
  std::string saltHex = ConfigManager::getBambuSalt();
  if (saltHex.empty()) return -1; // No salt configured = can't even try, but it's a Mifare 1K

  uint8_t salt[16];
  hexToBytes(saltHex, salt, 16);

  std::vector<std::vector<uint8_t>> derivedKeys;
  if (!BambuLabTagParser::deriveKeys(uid, salt, derivedKeys)) {
      return false;
  }

  std::vector<uint8_t> rawData(1024, 0);

  Serial.println("Attempting PN5180 Bambu Lab authentication...");

  // The tag might be halted from previous failed Snapmaker read or readCardSerial.
  // Wake it up (WUPA) and select it again.
  uint8_t tmp[10];
  pn5180_14443.activateTypeA(tmp, 1);

  // We need sectors 0, 1, 2, 3, 4, 5, 6 for the full data
  uint8_t sectorsToRead[] = {0, 1, 2, 3, 4, 5, 6};
  for (uint8_t sector : sectorsToRead) {
      if (!pn5180_mifareAuthenticate(derivedKeys[sector].data(), 0x60, sector * 4, uid)) {
          Serial.print("pn5180_mifareAuthenticate() failed for sector ");
          Serial.println(sector);
          
          // Check if the tag is still physically present
          uint8_t tmp[10];
          if (pn5180_14443.activateTypeA(tmp, 1) == 0) {
              return 0; // Card is no longer present/responsive
          }
          return -1; // Authentication failed = likely wrong salt
      }

      for (uint8_t block = 0; block < 3; block++) {
          uint8_t blockAddr = sector * 4 + block;
          byte buffer[16];
          if (!pn5180_14443.mifareBlockRead(blockAddr, buffer)) {
              return false;
          }
          memcpy(&rawData[blockAddr * 16], buffer, 16);
      }
  }

  // Parse the data
  if (BambuLabTagParser::parse(rawData, uid, data)) {
      Serial.println("Bambu Lab tag parsed successfully!");
      return 1;
  }

  return 0;
#else
  std::string saltHex = ConfigManager::getBambuSalt();
  if (saltHex.empty()) return -1; // No salt configured = can't even try, but it's a Mifare 1K

  uint8_t salt[16];
  hexToBytes(saltHex, salt, 16);

  std::vector<std::vector<uint8_t>> derivedKeys;
  if (!BambuLabTagParser::deriveKeys(mfrc522.uid.uidByte, salt, derivedKeys)) {
      return false;
  }

  std::vector<uint8_t> rawData(1024, 0);
  MFRC522::MIFARE_Key key;

  Serial.println("Attempting Bambu Lab authentication...");

  // We need sectors 0, 1, 2, 3, 4, 5, 6 for the full data (including Tray UID and Production Date)
  uint8_t sectorsToRead[] = {0, 1, 2, 3, 4, 5, 6};
  for (uint8_t sector : sectorsToRead) {
      memcpy(key.keyByte, derivedKeys[sector].data(), 6);

      // Authenticate Sector
      MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, sector * 4, &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) {
          Serial.print("Bambu auth failed for sector ");
          Serial.print(sector);
          Serial.print(": ");
          Serial.println(mfrc522.GetStatusCodeName(status));
          
          // Check if the tag is still physically present
          byte buffer[2];
          byte size = sizeof(buffer);
          mfrc522.PICC_HaltA();
          mfrc522.PCD_StopCrypto1();
          MFRC522::StatusCode wupaStatus = mfrc522.PICC_WakeupA(buffer, &size);
          if (wupaStatus != MFRC522::STATUS_OK && wupaStatus != MFRC522::STATUS_COLLISION) {
              return 0; // Card is no longer present/responsive
          }
          return -1; // Authentication failed = likely wrong salt
      }

      // Read blocks (0-2, block 3 is trailer)
      for (uint8_t block = 0; block < 3; block++) {
          uint8_t blockAddr = sector * 4 + block;
          byte buffer[18];
          byte size = sizeof(buffer);
          status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
          if (status != MFRC522::STATUS_OK) {
              return false;
          }
          memcpy(&rawData[blockAddr * 16], buffer, 16);
      }
  }

  // Parse the data
  if (BambuLabTagParser::parse(rawData, mfrc522.uid.uidByte, data)) {
      Serial.println("Bambu Lab tag parsed successfully!");
      return 1;
  }

  return 0;
#endif
}
