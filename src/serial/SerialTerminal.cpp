#include "SerialTerminal.h"
#include "../config/ConfigManager.h"

String SerialTerminal::rxBuffer = "";

void SerialTerminal::init() {
  // Serial should already be initialized in main setup,
  // but we can print a welcome message.
  Serial.println();
  Serial.println("=========================================");
  Serial.println("  CheapSpoolDisplay Serial Terminal");
  Serial.println("  Type 'help' for a list of commands.");
  Serial.println("=========================================");
  Serial.print("> ");
}

void SerialTerminal::tick() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    // Echo back (optional, helps with basic terminals)
    Serial.print(c);

    if (c == '\n' || c == '\r') {
      if (rxBuffer.length() > 0) {
        Serial.println(); // move to new line
        processCommand(rxBuffer);
        rxBuffer = "";
        Serial.print("> ");
      }
    } else {
      // Ignore backspace/delete for simplicity in this basic implementation,
      // or we could handle it:
      if (c == '\b' || c == 0x7f) {
        if (rxBuffer.length() > 0) {
          rxBuffer.remove(rxBuffer.length() - 1);
          // Erase character on terminal
          Serial.print(" \b");
        }
      } else {
        rxBuffer += c;
      }
    }
  }
}

void SerialTerminal::processCommand(const String &cmdLine) {
  String cmd = cmdLine;
  cmd.trim();

  if (cmd.length() == 0)
    return;

  if (cmd.equalsIgnoreCase("help") || cmd == "?") {
    Serial.println("Available commands:");
    Serial.println("  set ssid <name>");
    Serial.println("  set pass <password>");
    Serial.println("  set url <http://...>");
    Serial.println("  get config");
    Serial.println("  format (Erases all settings)");
    return;
  }

  if (cmd.equalsIgnoreCase("get config")) {
    Serial.println("[Current Config]");
    Serial.printf("SSID  : %s\n", ConfigManager::getWifiSSID().c_str());
    Serial.printf("PASS  : %s\n", ConfigManager::getWifiPass().c_str());
    Serial.printf("URL   : %s\n", ConfigManager::getWebhookUrl().c_str());
    return;
  }

  if (cmd.equalsIgnoreCase("format")) {
    ConfigManager::setWifiSSID("");
    ConfigManager::setWifiPass("");
    ConfigManager::setWebhookUrl("");
    Serial.println("Configuration formatted/erased.");
    return;
  }

  if (cmd.startsWith("set ")) {
    int firstSpace = cmd.indexOf(' ');
    int secondSpace = cmd.indexOf(' ', firstSpace + 1);

    if (secondSpace == -1) {
      Serial.println("Error: Missing value. Usage: set <key> <value>");
      return;
    }

    String key = cmd.substring(firstSpace + 1, secondSpace);
    String value = cmd.substring(secondSpace + 1);
    key.trim();
    value.trim();

    if (key.equalsIgnoreCase("ssid")) {
      ConfigManager::setWifiSSID(value.c_str());
      Serial.println("SSID saved.");
    } else if (key.equalsIgnoreCase("pass")) {
      ConfigManager::setWifiPass(value.c_str());
      Serial.println("Password saved.");
    } else if (key.equalsIgnoreCase("url")) {
      ConfigManager::setWebhookUrl(value.c_str());
      Serial.println("Webhook URL saved.");
    } else {
      Serial.printf("Error: Unknown key '%s'\n", key.c_str());
    }
    return;
  }

  Serial.println("Unknown command. Type 'help'.");
}
