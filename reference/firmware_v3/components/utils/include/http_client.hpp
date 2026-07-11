#pragma once

#include <string>

namespace nova {

bool http_get(const char* tag, const char* url, std::string& body,
              int timeout_ms = 5000, int buffer_size = 4096);

}  // namespace nova
