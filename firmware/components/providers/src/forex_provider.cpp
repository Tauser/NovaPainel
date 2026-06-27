#include "forex_provider.hpp"

#include <cerrno>
#include <cstdlib>
#include <string>

#include "esp_log.h"
#include "http_client.hpp"

namespace nova {

namespace {
constexpr const char* kTag = "ForexProvider";
constexpr const char* kUrl = "https://economia.awesomeapi.com.br/json/last/USD-BRL";

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

bool ForexProvider::fetch(ForexSummary& out, uint32_t now_ms)
{
    std::string body;
    if (!http_get(kTag, kUrl, body)) {
        return false;
    }

    double usd_brl = 0.0;
    if (!parse_bid(body, usd_brl)) {
        ESP_LOGW(kTag, "JSON parse failed");
        return false;
    }

    out.usd_brl = usd_brl;
    out.usd_brl_valid = true;
    out.usd_brl_stale = false;
    out.usd_brl_source = DataSource::Live;
    out.usd_brl_last_update_ms = now_ms;

    ESP_LOGI(kTag, "USD/BRL=%.4f", usd_brl);
    return true;
}

}  // namespace nova
