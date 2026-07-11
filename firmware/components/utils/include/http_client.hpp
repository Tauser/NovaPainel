#pragma once

#include "status.hpp"

#include <cstddef>
#include <string>

namespace nova {

constexpr std::size_t kHttpBodyCapBytes = 48 * 1024;

struct HttpResponse {
    int status_code{0};
    std::string body{};
};

class HttpClient {
public:
    Result<HttpResponse> get(const std::string& /*url*/, std::size_t response_size) {
        if (response_size > kHttpBodyCapBytes) {
            return Result<HttpResponse>::failure(
                Status::error(StatusCode::Truncated, "http response exceeds cap"));
        }
        return Result<HttpResponse>::failure(
            Status::error(StatusCode::Unavailable, "http client not wired in Fase 1"));
    }
};

}  // namespace nova
