#include "forex_provider.hpp"

#include <cstring>
#include <string>

#include <cJSON.h>

#include "esp_log.h"
#include "http_client.hpp"

namespace nova {

namespace {
constexpr const char* kTag = "ForexProvider";
constexpr const char* kUrl = "https://economia.awesomeapi.com.br/json/last/USD-BRL";

bool read_numeric_field(const cJSON* object, const char* key, double& value)
{
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(const_cast<cJSON*>(object), key);
    if (cJSON_IsNumber(item)) {
        value = item->valuedouble;
        return true;
    }

    if (cJSON_IsString(item) && item->valuestring != nullptr) {
        const double parsed = std::strtod(item->valuestring, nullptr);
        if (parsed > 0.0 || std::strcmp(item->valuestring, "0") == 0 ||
            std::strcmp(item->valuestring, "0.0") == 0) {
            value = parsed;
            return true;
        }
    }

    ESP_LOGW(kTag, "JSON payload missing numeric field: %s", key);
    return false;
}

bool parse_quote(const std::string& json, ForexSummary& out, uint32_t now_ms)
{
    cJSON* root = cJSON_ParseWithLength(json.c_str(), json.size());
    if (!root) {
        const char* parse_error = cJSON_GetErrorPtr();
        if (parse_error) {
            ESP_LOGW(kTag, "JSON parse failed near: %.120s", parse_error);
        } else {
            ESP_LOGW(kTag, "JSON parse failed");
        }
        return false;
    }

    const cJSON* pair = cJSON_GetObjectItemCaseSensitive(root, "USDBRL");
    if (!cJSON_IsObject(pair)) {
        ESP_LOGW(kTag, "JSON payload missing USDBRL object");
        cJSON_Delete(root);
        return false;
    }

    double bid = 0.0;
    double high = 0.0;
    double low = 0.0;
    double change_pct = 0.0;
    if (!read_numeric_field(pair, "bid", bid) ||
        !read_numeric_field(pair, "high", high) ||
        !read_numeric_field(pair, "low", low)) {
        cJSON_Delete(root);
        return false;
    }

    const cJSON* pct_change = cJSON_GetObjectItemCaseSensitive(const_cast<cJSON*>(pair), "pctChange");
    if (cJSON_IsString(pct_change) && pct_change->valuestring != nullptr) {
        change_pct = std::strtod(pct_change->valuestring, nullptr);
    } else if (cJSON_IsNumber(pct_change)) {
        change_pct = pct_change->valuedouble;
    }

    if (bid <= 0.0) {
        ESP_LOGW(kTag, "JSON payload returned invalid bid");
        cJSON_Delete(root);
        return false;
    }

    out.usd_brl = bid;
    out.usd_brl_change_pct_24h = change_pct;
    out.usd_brl_high_24h = high;
    out.usd_brl_low_24h = low;
    out.usd_brl_valid = true;
    out.usd_brl_stale = false;
    out.usd_brl_source = DataSource::Live;
    out.usd_brl_last_update_ms = now_ms;

    cJSON_Delete(root);
    return true;
}

}  // namespace

bool ForexProvider::fetch(ForexSummary& out, uint32_t now_ms)
{
    std::string body;
    if (!http_get(kTag, kUrl, body)) {
        return false;
    }

    if (!parse_quote(body, out, now_ms)) {
        ESP_LOGW(kTag, "JSON parse failed");
        return false;
    }

    ESP_LOGI(kTag, "USD/BRL=%.4f high=%.4f low=%.4f",
             out.usd_brl, out.usd_brl_high_24h, out.usd_brl_low_24h);
    return true;
}

}  // namespace nova
