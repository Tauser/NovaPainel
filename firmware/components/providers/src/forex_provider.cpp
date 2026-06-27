#include "forex_provider.hpp"

#include <cerrno>
#include <cstdlib>
#include <string>
#include <utility>

#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "ForexProvider";
constexpr const char* kUrl = "https://economia.awesomeapi.com.br/json/last/USD-BRL";

struct HttpBody {
    std::string data;
};

esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
    auto* body = static_cast<HttpBody*>(evt->user_data);
    if (!body) {
        return ESP_OK;
    }

    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        body->data.append(static_cast<const char*>(evt->data), evt->data_len);
    }
    return ESP_OK;
}

bool fetch_http(const char* url, std::string& body)
{
    HttpBody payload;
    esp_http_client_config_t config{};
    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 5000;
    config.buffer_size = 1024;
    config.event_handler = http_event_handler;
    config.user_data = &payload;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGW(kTag, "esp_http_client_init failed");
        return false;
    }

    const esp_err_t err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200 || payload.data.empty()) {
        ESP_LOGW(kTag, "HTTP failed: err=%s status=%d", esp_err_to_name(err), status);
        return false;
    }

    body = std::move(payload.data);
    return true;
}

bool parse_bid(const std::string& json, double& value)
{
    const std::string needle = "\"bid\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return false;
    }

    const std::size_t colon_pos = json.find(':', key_pos + needle.size());
    if (colon_pos == std::string::npos) {
        return false;
    }

    const std::size_t quote_start = json.find('"', colon_pos + 1);
    if (quote_start == std::string::npos) {
        return false;
    }

    const char* start = json.c_str() + quote_start + 1;
    char* end = nullptr;
    errno = 0;
    const double parsed = std::strtod(start, &end);
    if (end == start || errno == ERANGE || parsed <= 0.0) {
        return false;
    }

    value = parsed;
    return true;
}

}  // namespace

bool ForexProvider::fetch(double& usd_brl, uint32_t)
{
    std::string body;
    if (!fetch_http(kUrl, body)) {
        return false;
    }

    if (!parse_bid(body, usd_brl)) {
        ESP_LOGW(kTag, "JSON parse failed");
        return false;
    }

    ESP_LOGI(kTag, "USD/BRL=%.4f", usd_brl);
    return true;
}

}  // namespace nova
