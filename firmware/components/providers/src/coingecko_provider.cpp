#include "coingecko_provider.hpp"

#include <string>

#include <cJSON.h>

#include "esp_log.h"
#include "http_client.hpp"

namespace nova {

namespace {
constexpr const char* kTag = "CoinGeckoProvider";
constexpr const char* kUrl =
    "https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=bitcoin&price_change_percentage=24h";

bool read_number(const cJSON* object, const char* key, double& value)
{
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(const_cast<cJSON*>(object), key);
    if (!cJSON_IsNumber(item)) {
        ESP_LOGW(kTag, "JSON payload missing numeric field: %s", key);
        return false;
    }
    value = item->valuedouble;
    return true;
}

}  // namespace

bool CoinGeckoProvider::fetch(CryptoSummary& out, uint32_t now_ms)
{
    std::string body;
    if (!http_get(kTag, kUrl, body)) {
        return false;
    }

    cJSON* root = cJSON_ParseWithLength(body.c_str(), body.size());
    if (!root) {
        const char* parse_error = cJSON_GetErrorPtr();
        if (parse_error) {
            ESP_LOGW(kTag, "JSON parse failed near: %.120s", parse_error);
        } else {
            ESP_LOGW(kTag, "JSON parse failed");
        }
        return false;
    }

    const cJSON* first = cJSON_GetArrayItem(root, 0);
    if (!cJSON_IsObject(first)) {
        ESP_LOGW(kTag, "JSON payload missing market object");
        cJSON_Delete(root);
        return false;
    }

    double btc_usd = 0.0;
    double change_24h = 0.0;
    double high_24h = 0.0;
    double low_24h = 0.0;
    double volume_24h = 0.0;
    if (!read_number(first, "current_price", btc_usd) ||
        !read_number(first, "price_change_percentage_24h", change_24h) ||
        !read_number(first, "high_24h", high_24h) ||
        !read_number(first, "low_24h", low_24h) ||
        !read_number(first, "total_volume", volume_24h)) {
        cJSON_Delete(root);
        return false;
    }

    out.btc_usd = btc_usd;
    out.btc_change_24h = change_24h;
    out.btc_open_24h = change_24h <= -99.0 ? 0.0 : (btc_usd / (1.0 + (change_24h / 100.0)));
    out.btc_high_24h = high_24h;
    out.btc_low_24h = low_24h;
    out.btc_volume_24h = volume_24h;
    out.valid = true;
    out.stale = false;
    out.source = DataSource::Live;
    out.last_update_ms = now_ms;
    cJSON_Delete(root);

    ESP_LOGI(kTag, "BTC=%.2f change=%.2f%% high=%.2f low=%.2f",
             out.btc_usd, out.btc_change_24h, out.btc_high_24h, out.btc_low_24h);
    return true;
}

}  // namespace nova
