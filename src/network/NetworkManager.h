#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "../data/OpenSpool.h"
#include <string>

class NetworkManager {
public:
  static void connectWiFi();
  static bool sendWebhookPayload(const OpenSpoolData &data,
                                 int toolhead_id = 0);
  static bool fetchSpoolmanData(OpenSpoolData &data);
};

#endif // NETWORK_MANAGER_H
