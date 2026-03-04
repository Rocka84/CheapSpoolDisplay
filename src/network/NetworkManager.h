#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <string>

class NetworkManager {
public:
  static void connectWiFi();
  static bool sendWebhookPayload(const std::string &spool_id, int toolhead_id);
};

#endif // NETWORK_MANAGER_H
