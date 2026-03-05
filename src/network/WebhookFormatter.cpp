#include "WebhookFormatter.h"
#include <sstream>

std::string WebhookFormatter::formatUrl(const std::string &url,
                                        const std::string &spool_id,
                                        int toolhead_id) {
  if (url.empty()) {
    return url;
  }

  std::string result = url;
  size_t pos;

  // Replace all occurrences of {spool_id}
  while ((pos = result.find("{spool_id}")) != std::string::npos) {
    result.replace(pos, 10, spool_id);
  }

  // Convert toolhead_id to string
  std::stringstream ss;
  ss << toolhead_id;
  std::string tool_str = ss.str();

  // Replace all occurrences of {toolhead}
  while ((pos = result.find("{toolhead}")) != std::string::npos) {
    result.replace(pos, 10, tool_str);
  }

  return result;
}
