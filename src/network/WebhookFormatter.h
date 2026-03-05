#ifndef WEBHOOK_FORMATTER_H
#define WEBHOOK_FORMATTER_H

#include <string>

class WebhookFormatter {
public:
  static std::string formatUrl(const std::string &url,
                               const std::string &spool_id, int toolhead_id);
};

#endif // WEBHOOK_FORMATTER_H
