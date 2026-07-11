#include "coingecko_provider.hpp"

#include <cstdio>
#include <string>

#include <cJSON.h>

#include "esp_log.h"
#include "http_client.hpp"

namespace nova {

namespace {
constexpr const char* kTag     = "CoinGeckoProvider";
constexpr const char* kUrl     =
    "https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=bitcoin&price_change_percentage=24h";
constexpr const char* kOhlcFmt =
    "https://api.coingecko.com/api/v3/coins/bitcoin/ohlc?vs_currency=usd&days=%d";

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

bool CoinGeckoProvider::fetch_ohlc(OhlcSeries& out, OhlcPeriod period, uint32_t now_ms)
{
    int days = 1;
    switch (period) {
        case OhlcPeriod::H1: days = 1;   break;  // ~48 candles 30min
        case OhlcPeriod::H4: days = 14;  break;  // ~84 candles 4h   (10-14 dias)
        case OhlcPeriod::D1: days = 180; break;  // ~180 daily → aparadas a 120 pelo kOhlcMaxCandles
        case OhlcPeriod::W1: days = 365; break;  // ~52 candles weekly (1 ano)
    }

    char url[128];
    std::snprintf(url, sizeof(url), kOhlcFmt, days);

    std::string body;
    if (!http_get(kTag, url, body)) {
        return false;
    }

    cJSON* root = cJSON_ParseWithLength(body.c_str(), body.size());
    if (!cJSON_IsArray(root)) {
        ESP_LOGW(kTag, "OHLC: expected JSON array");
        cJSON_Delete(root);
        return false;
    }

    const int total = cJSON_GetArraySize(root);
    if (total == 0) {
        ESP_LOGW(kTag, "OHLC: empty array");
        cJSON_Delete(root);
        return false;
    }

    // Take up to kOhlcMaxCandles from the tail (most recent entries).
    const int start = (total > static_cast<int>(kOhlcMaxCandles))
                      ? total - static_cast<int>(kOhlcMaxCandles)
                      : 0;

    out.candles.clear();
    out.candles.reserve(static_cast<std::size_t>(total - start));
    out.period = period;

    for (int i = start; i < total; ++i) {
        cJSON* entry = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsArray(entry) || cJSON_GetArraySize(entry) < 5) {
            continue;
        }

        OhlcCandle c{};
        c.ts_ms = static_cast<int64_t>(cJSON_GetArrayItem(entry, 0)->valuedouble);
        c.o     = static_cast<float>(cJSON_GetArrayItem(entry, 1)->valuedouble);
        c.h     = static_cast<float>(cJSON_GetArrayItem(entry, 2)->valuedouble);
        c.l     = static_cast<float>(cJSON_GetArrayItem(entry, 3)->valuedouble);
        c.c     = static_cast<float>(cJSON_GetArrayItem(entry, 4)->valuedouble);
        c.v     = 0.0f;  // CoinGecko /ohlc endpoint does not return volume
        out.candles.push_back(c);
    }

    cJSON_Delete(root);

    if (out.candles.empty()) {
        ESP_LOGW(kTag, "OHLC: no valid candles parsed");
        return false;
    }

    out.valid           = true;
    out.stale           = false;
    out.source          = DataSource::Live;
    out.last_update_ms  = now_ms;

    ESP_LOGI(kTag, "OHLC: %u candles (period=%d, days=%d) first_o=%.2f last_c=%.2f",
             static_cast<unsigned>(out.candles.size()), static_cast<int>(period), days,
             out.candles.front().o, out.candles.back().c);
    return true;
}

}  // namespace nova
