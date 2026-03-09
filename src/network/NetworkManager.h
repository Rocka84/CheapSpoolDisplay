#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "../data/OpenSpool.h"
#include <string>

class NetworkManager {
public:
  static void connectWiFi();
  static bool sendWebhookPayload(const std::string &spool_id, int toolhead_id);
  static bool fetchSpoolmanData(OpenSpoolData &data);
};

#endif // NETWORK_MANAGER_H
