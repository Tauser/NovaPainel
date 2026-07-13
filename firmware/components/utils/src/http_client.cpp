#include "http_client.hpp"

#if defined(ESP_PLATFORM)

#include <cstring>
#include <mutex>

#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"

namespace nova {
namespace {
constexpr const char* kTag = "HttpClient";

// Serializa TODO GET HTTPS do firmware -- ver comentário do header.
std::mutex& http_gate() {
    static std::mutex gate;
    return gate;
}

struct HttpBody {
    char* buf;
    std::size_t len;
    std::size_t cap;
    bool truncated;
};

esp_err_t http_event_handler(esp_http_client_event_t* evt) {
    if (evt->event_id != HTTP_EVENT_ON_DATA || evt->user_data == nullptr) {
        return ESP_OK;
    }
    auto* body = static_cast<HttpBody*>(evt->user_data);
    const std::size_t available = body->cap - body->len;
    const std::size_t to_copy =
        (static_cast<std::size_t>(evt->data_len) <= available) ? static_cast<std::size_t>(evt->data_len) : available;
    if (to_copy > 0) {
        std::memcpy(body->buf + body->len, evt->data, to_copy);
        body->len += to_copy;
    }
    if (to_copy < static_cast<std::size_t>(evt->data_len)) {
        body->truncated = true;
    }
    return ESP_OK;
}

}  // namespace

Result<HttpResponse> HttpClient::get(const std::string& url, uint32_t timeout_ms) {
    std::lock_guard<std::mutex> serialize(http_gate());

    // SRAM interna, não PSRAM: o DMA do display precisa de acesso exclusivo
    // à PSRAM durante o download (RESOURCE-BUDGET.md §1/§2).
    char* raw = static_cast<char*>(
        heap_caps_malloc(kHttpBodyCapBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (raw == nullptr) {
        return Result<HttpResponse>::failure(
            Status::error(StatusCode::Unavailable, "http body allocation failed (SRAM OOM)"));
    }
    HttpBody body{raw, 0, kHttpBodyCapBytes, false};

    esp_http_client_config_t config{};
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = static_cast<int>(timeout_ms);
    config.event_handler = http_event_handler;
    config.user_data = &body;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        heap_caps_free(raw);
        return Result<HttpResponse>::failure(
            Status::error(StatusCode::Unavailable, "esp_http_client_init failed"));
    }

    const esp_err_t err = esp_http_client_perform(client);
    const int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGW(kTag, "request failed: %s", esp_err_to_name(err));
        heap_caps_free(raw);
        return Result<HttpResponse>::failure(Status::error(StatusCode::Unavailable, "http request failed"));
    }
    if (body.truncated) {
        ESP_LOGW(kTag, "response exceeds %u B cap", static_cast<unsigned>(kHttpBodyCapBytes));
        heap_caps_free(raw);
        return Result<HttpResponse>::failure(
            Status::error(StatusCode::Truncated, "http response exceeds cap"));
    }

    HttpResponse response{};
    response.status_code = status_code;
    response.body.assign(raw, body.len);
    heap_caps_free(raw);
    return Result<HttpResponse>::success(std::move(response));
}

}  // namespace nova

#else

namespace nova {

Result<HttpResponse> HttpClient::get(const std::string& /*url*/, uint32_t /*timeout_ms*/) {
    return Result<HttpResponse>::failure(
        Status::error(StatusCode::Unavailable, "http client not available on host"));
}

}  // namespace nova

#endif
