#include <vector>
#include "NFCReader.h"

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
    // Increase antenna gain to moderate level (38dB) to improve range without saturation
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_avg);
  }
}

bool NFCReader::scanForTag(OpenSpoolData &data) {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    Serial.println("PICC_ReadCardSerial failed");
    return false;
  }

  Serial.println("Tag detected! Attempting to read payload...");

  std::string ndefJson = readNDEFPayload();
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  if (ndefJson.empty()) {
    Serial.println("Error: NDEF payload was empty or read failed.");
    return false;
  }

  Serial.println("Payload extracted. Attempting JSON parse...");
  bool parsed = OpenSpoolParser::parseJson(ndefJson, data);
  if (!parsed) {
    Serial.printf("JSON parse failed. Raw payload:\n%s\n", ndefJson.c_str());
  }
  return parsed;
}

bool NFCReader::writeTag(const OpenSpoolData &data) {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  std::string json = OpenSpoolParser::toJson(data);
  bool success = writeNDEFPayload(json);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
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

  // NTAG215 pages start user memory at page 4. Read up to page 128 (covers up
  // to 496 bytes)
  for (byte page = 4; page < 128; page += 4) {
    byte size = sizeof(buffer);
    MFRC522::StatusCode status = mfrc522.MIFARE_Read(page, buffer, &size);
    if (status == MFRC522::STATUS_OK) {
      for (int i = 0; i < 16; i++) {
        payload += (char)buffer[i];
      }

      // Dump the first chunk so we can see what physical data is on the tag
      if (page == 4) {
        Serial.print("Raw Hex (Page 4-7): ");
        for (int i = 0; i < 16; i++) {
          if (buffer[i] < 0x10)
            Serial.print("0");
          Serial.print(buffer[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
      }

    } else {
      Serial.printf("MIFARE_Read failed at page %d with status: %s\n", page,
                    mfrc522.GetStatusCodeName(status));
      break;
    }
  }

  Serial.printf("Total payload length read from tag: %d bytes\n",
                payload.length());
  // Look for JSON start and end
  size_t start = payload.find("{");
  size_t end = payload.rfind("}");
  Serial.printf("Found '{' at %d, '}' at %d\n", (int)start, (int)end);

  if (start != std::string::npos && end != std::string::npos && end > start) {
    std::string jsonStr = payload.substr(start, end - start + 1);
    Serial.printf("Extracted JSON fragment:\n%s\n", jsonStr.c_str());
    return jsonStr;
  }

  return "";
}

bool NFCReader::writeNDEFPayload(const std::string &json) {
  // NDEF formatting for NTAG215
  // Page 3: CC (Capability Container) - E1 10 3E 00 for NTAG215 (504 bytes)
  byte cc[4] = {0xE1, 0x10, 0x3E, 0x00};
  MFRC522::StatusCode status = mfrc522.MIFARE_Ultralight_Write(3, cc, 4);
  if (status != MFRC522::STATUS_OK) {
    Serial.printf("CC Write failed: %s\n", mfrc522.GetStatusCodeName(status));
    return false;
  }

  // Build NDEF Message
  // TNF 0x02 (Mime Media), Type "application/json"
  std::string type = "application/json";
  size_t payloadLen = json.length();
  size_t ndefLen = 3 + type.length() + payloadLen; // Header + TypeLength + PayloadLength + Type + Payload

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
  ndef.push_back((byte)type.length());
  ndef.push_back((byte)payloadLen);
  for (char c : type) ndef.push_back((byte)c);
  for (char c : json) ndef.push_back((byte)c);
  ndef.push_back(0xFE); // Terminator TLV

  // Pad to 4-byte pages
  while (ndef.size() % 4 != 0) {
    ndef.push_back(0x00);
  }

  // Write pages starting at 4
  for (size_t i = 0; i < ndef.size(); i += 4) {
    byte page = 4 + (i / 4);
    if (page >= 130) break; // NTAG215 limit
    status = mfrc522.MIFARE_Ultralight_Write(page, &ndef[i], 4);
    if (status != MFRC522::STATUS_OK) {
      Serial.printf("Write failed at page %d: %s\n", page, mfrc522.GetStatusCodeName(status));
      return false;
    }
  }

  return true;
}
