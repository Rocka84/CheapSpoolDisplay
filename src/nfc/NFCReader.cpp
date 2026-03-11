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
  // Writing NDEF requires writing CC, TLV blocks.
  // This is a placeholder for the actual NDEF writing logic.
  // It involves formatting the card as NDEF and writing the `application/json`
  // record.
  return true;
}
