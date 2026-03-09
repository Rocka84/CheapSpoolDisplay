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
    Serial.println("  set wifi <ssid> <password>");
    Serial.println("  set webhook <http://...>");
    Serial.println("  set spoolman <http://...>");
    Serial.println("  set tools <1-6>");
    Serial.println("  set timeout <seconds>");
    Serial.println("  get config");
    Serial.println("  format (Erases all settings)");
    return;
  }

  if (cmd.equalsIgnoreCase("get config")) {
    Serial.println("[Current Config]");
    Serial.printf("SSID    : %s\n", ConfigManager::getWifiSSID().c_str());
    std::string pass = ConfigManager::getWifiPass();
    std::string redactedPass = pass.empty() ? "" : "********";
    Serial.printf("PASS    : %s\n", redactedPass.c_str());
    Serial.printf("WEBHOOK : %s\n", ConfigManager::getWebhook().c_str());
    Serial.printf("SPOOLMAN: %s\n", ConfigManager::getSpoolmanUrl().c_str());
    Serial.printf("TOOLS   : %d\n", ConfigManager::getNumTools());
    Serial.printf("TIMEOUT : %d sec\n", ConfigManager::getScreenTimeout());
    return;
  }

  if (cmd.equalsIgnoreCase("format")) {
    ConfigManager::setWifiSSID("");
    ConfigManager::setWifiPass("");
    ConfigManager::setWebhook("");
    ConfigManager::setSpoolmanUrl("");
    ConfigManager::setNumTools(1);
    ConfigManager::setScreenTimeout(60);
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

    if (key.equalsIgnoreCase("wifi")) {
      int thirdSpace = cmd.indexOf(' ', secondSpace + 1);
      if (thirdSpace == -1) {
        // Assume the rest of the string is just the SSID (no password network)
        ConfigManager::setWifiSSID(value.c_str());
        ConfigManager::setWifiPass("");
        Serial.println("Saved open Wi-Fi network (no password).");
      } else {
        String ssid = cmd.substring(secondSpace + 1, thirdSpace);
        String pass = cmd.substring(thirdSpace + 1);
        ssid.trim();
        pass.trim();
        ConfigManager::setWifiSSID(ssid.c_str());
        ConfigManager::setWifiPass(pass.c_str());
        Serial.println("Wi-Fi credentials saved.");
      }
    } else if (key.equalsIgnoreCase("webhook")) {
      ConfigManager::setWebhook(value.c_str());
      Serial.println("Webhook saved.");
    } else if (key.equalsIgnoreCase("spoolman")) {
      ConfigManager::setSpoolmanUrl(value.c_str());
      Serial.println("Spoolman URL saved.");
    } else if (key.equalsIgnoreCase("tools")) {
      int num = value.toInt();
      if (num >= 1 && num <= 6) {
        ConfigManager::setNumTools(num);
        Serial.printf("Number of tools saved: %d\n", num);
      } else {
        Serial.println("Error: Number of tools must be between 1 and 6.");
      }
    } else if (key.equalsIgnoreCase("timeout")) {
      int num = value.toInt();
      if (num >= 10 && num <= 3600) {
        ConfigManager::setScreenTimeout(num);
        Serial.printf("Screen timeout saved: %d seconds\n", num);
      } else {
        Serial.println(
            "Error: Screen timeout must be between 10 and 3600 seconds.");
      }
    } else {
      Serial.printf("Error: Unknown key '%s'\n", key.c_str());
    }
    return;
  }

  Serial.println("Unknown command. Type 'help'.");
}
