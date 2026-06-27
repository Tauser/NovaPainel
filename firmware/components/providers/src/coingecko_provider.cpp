#include "coingecko_provider.hpp"

#include <cerrno>
#include <cstdlib>
#include <string>

#include "esp_log.h"
#include "http_client.hpp"

namespace nova {

namespace {
constexpr const char* kTag = "CoinGeckoProvider";
constexpr const char* kUrl =
    "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true";

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

}  // namespace

bool CoinGeckoProvider::fetch(CryptoSummary& out, uint32_t now_ms)
{
    std::string body;
    if (!http_get(kTag, kUrl, body)) {
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
