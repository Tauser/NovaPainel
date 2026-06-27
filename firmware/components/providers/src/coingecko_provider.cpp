#include "coingecko_provider.hpp"

#include <cerrno>
#include <cstdlib>
#include <utility>
#include <string>

#include "esp_err.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "CoinGeckoProvider";
constexpr const char* kUrl =
    "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true";

struct HttpBody {
    std::string data;
};

esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
    auto* body = static_cast<HttpBody*>(evt->user_data);
    if (!body) {
        return ESP_OK;
    }

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            body->data.append(static_cast<const char*>(evt->data), evt->data_len);
            break;
        default:
            break;
    }
    return ESP_OK;
}

const char* find_value_start(const std::string& json, const char* key)
{
    const std::string needle = std::string("\"") + key + "\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return nullptr;
    }

    const std::size_t colon_pos = json.find(':', key_pos + needle.size());
    if (colon_pos == std::string::npos) {
        return nullptr;
    }

    const char* value = json.c_str() + colon_pos + 1;
    while (*value == ' ' || *value == '\n' || *value == '\r' || *value == '\t') {
        ++value;
    }
    return value;
}

bool parse_double(const std::string& json, const char* key, double& value)
{
    const char* start = find_value_start(json, key);
    if (!start) {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    const double parsed = std::strtod(start, &end);
    if (end == start || errno == ERANGE) {
        return false;
    }
    value = parsed;
    return true;
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

    const esp_err_t open_err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (open_err != ESP_OK || status != 200 || payload.data.empty()) {
        ESP_LOGW(kTag, "HTTP failed: err=%s status=%d",
                 esp_err_to_name(open_err), status);
        return false;
    }

    body = std::move(payload.data);
    return true;
}

}  // namespace

bool CoinGeckoProvider::fetch(MarketSummary& out, uint32_t now_ms)
{
    std::string body;
    if (!fetch_http(kUrl, body)) {
        return false;
    }

    double btc_usd = 0.0;
    double change_24h = 0.0;
    if (!parse_double(body, "usd", btc_usd) ||
        !parse_double(body, "usd_24h_change", change_24h)) {
        ESP_LOGW(kTag, "JSON parse failed");
        return false;
    }

    out.btc_usd = btc_usd;
    out.btc_change_24h = change_24h;
    out.valid = true;
    out.stale = false;
    out.source = DataSource::Live;
    out.last_update_ms = now_ms;

    ESP_LOGI(kTag, "BTC=%.2f change=%.2f%%",
             out.btc_usd, out.btc_change_24h);
    return true;
}

}  // namespace nova
