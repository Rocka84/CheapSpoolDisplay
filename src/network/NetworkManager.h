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
  static bool fetchSpoolmanList(int page, int limit, std::vector<SpoolmanItem>& items, int& total_count);
};

#endif // NETWORK_MANAGER_H
