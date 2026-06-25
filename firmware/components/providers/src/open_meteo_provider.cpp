// NovaPainel - providers/open_meteo_provider.cpp
// Hardware-only: esp_http_client + mbedtls cert bundle + cJSON, none of
// which have a host shim (see tools/scripts/host_check.sh SKIP_FILES). Same
// NTP/TLS prerequisite as CoinGeckoProvider (ADR-0021).
#include "open_meteo_provider.hpp"

#include <cstring>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"

namespace nova {

namespace {
constexpr const char* kTag = "OpenMeteoProvider";

// Brasília, DF (no location step in the wizard yet - ADR-0022). Open-Meteo
// needs no API key, unlike CoinGecko's Demo key (ADR-0006).
constexpr const char* kUrl =
    "https://api.open-meteo.com/v1/forecast"
    "?latitude=-15.78&longitude=-47.93"
    "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,apparent_temperature"
    "&daily=temperature_2m_max,temperature_2m_min,uv_index_max"
    "&timezone=America%2FSao_Paulo";

// Bigger now that the response also carries the "daily" block (today's
// high/low/UV) on top of "current".
constexpr int kMaxResponseBytes = 1536;

struct FetchContext {
    char buf[kMaxResponseBytes];
    int  len{0};
};

esp_err_t http_event_handler(esp_http_client_event_t* evt) {
    if (evt->event_id != HTTP_EVENT_ON_DATA) return ESP_OK;
    auto* ctx = static_cast<FetchContext*>(evt->user_data);
    if (ctx->len + evt->data_len < kMaxResponseBytes) {
        std::memcpy(ctx->buf + ctx->len, evt->data, evt->data_len);
        ctx->len += evt->data_len;
    }
    return ESP_OK;
}

// WMO weather codes (https://open-meteo.com/en/docs - "WMO Weather
// interpretation codes"), bucketed down to WeatherCondition.
WeatherCondition condition_for_wmo_code(int code) {
    if (code == 0) return WeatherCondition::Clear;
    if (code >= 1 && code <= 3) return WeatherCondition::Cloudy;
    if (code == 45 || code == 48) return WeatherCondition::Fog;
    if (code >= 51 && code <= 57) return WeatherCondition::Drizzle;
    if ((code >= 61 && code <= 67) || (code >= 80 && code <= 82)) return WeatherCondition::Rain;
    if ((code >= 71 && code <= 77) || code == 85 || code == 86) return WeatherCondition::Snow;
    if (code >= 95 && code <= 99) return WeatherCondition::Thunderstorm;
    return WeatherCondition::Unknown;
}
}  // namespace

Result<WeatherSummary> OpenMeteoProvider::fetch_current() {
    FetchContext ctx{};

    esp_http_client_config_t config = {};
    config.url = kUrl;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.event_handler = &http_event_handler;
    config.user_data = &ctx;
    config.timeout_ms = 8000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    const esp_err_t err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200) {
        ESP_LOGW(kTag, "http failed: err=%s status=%d", esp_err_to_name(err), status);
        return Result<WeatherSummary>::fail(ErrorCode::Network, "open-meteo http failed");
    }

    ctx.buf[ctx.len < kMaxResponseBytes ? ctx.len : kMaxResponseBytes - 1] = '\0';

    cJSON* root = cJSON_Parse(ctx.buf);
    if (!root) {
        ESP_LOGW(kTag, "json parse failed: '%s'", ctx.buf);
        return Result<WeatherSummary>::fail(ErrorCode::Parse, "open-meteo json parse failed");
    }

    cJSON* current = cJSON_GetObjectItemCaseSensitive(root, "current");
    cJSON* temp = current ? cJSON_GetObjectItemCaseSensitive(current, "temperature_2m") : nullptr;
    cJSON* humidity = current ? cJSON_GetObjectItemCaseSensitive(current, "relative_humidity_2m") : nullptr;
    cJSON* code = current ? cJSON_GetObjectItemCaseSensitive(current, "weather_code") : nullptr;
    cJSON* wind = current ? cJSON_GetObjectItemCaseSensitive(current, "wind_speed_10m") : nullptr;
    cJSON* feels_like = current ? cJSON_GetObjectItemCaseSensitive(current, "apparent_temperature") : nullptr;

    if (!cJSON_IsNumber(temp) || !cJSON_IsNumber(code)) {
        ESP_LOGW(kTag, "json missing temperature_2m/weather_code fields");
        cJSON_Delete(root);
        return Result<WeatherSummary>::fail(ErrorCode::Parse, "open-meteo json missing fields");
    }

    // "daily" arrays are indexed by day, with today always at [0] (the
    // request has no start_date/forecast_days override, so Open-Meteo
    // defaults to starting today). Optional - the UI shows "--" rather than
    // failing the whole fetch if this block is ever missing.
    cJSON* daily = cJSON_GetObjectItemCaseSensitive(root, "daily");
    cJSON* temp_max_arr = daily ? cJSON_GetObjectItemCaseSensitive(daily, "temperature_2m_max") : nullptr;
    cJSON* temp_min_arr = daily ? cJSON_GetObjectItemCaseSensitive(daily, "temperature_2m_min") : nullptr;
    cJSON* uv_arr = daily ? cJSON_GetObjectItemCaseSensitive(daily, "uv_index_max") : nullptr;
    cJSON* temp_max0 = cJSON_IsArray(temp_max_arr) ? cJSON_GetArrayItem(temp_max_arr, 0) : nullptr;
    cJSON* temp_min0 = cJSON_IsArray(temp_min_arr) ? cJSON_GetArrayItem(temp_min_arr, 0) : nullptr;
    cJSON* uv0 = cJSON_IsArray(uv_arr) ? cJSON_GetArrayItem(uv_arr, 0) : nullptr;

    WeatherSummary s{};
    s.temperature_c = temp->valuedouble;
    s.feels_like_c = cJSON_IsNumber(feels_like) ? feels_like->valuedouble : temp->valuedouble;
    s.temp_max_c = cJSON_IsNumber(temp_max0) ? temp_max0->valuedouble : temp->valuedouble;
    s.temp_min_c = cJSON_IsNumber(temp_min0) ? temp_min0->valuedouble : temp->valuedouble;
    s.uv_index = cJSON_IsNumber(uv0) ? uv0->valuedouble : 0.0;
    s.humidity_pct = cJSON_IsNumber(humidity) ? static_cast<int>(humidity->valuedouble) : 0;
    s.wind_speed_kmh = cJSON_IsNumber(wind) ? wind->valuedouble : 0.0;
    s.condition = condition_for_wmo_code(static_cast<int>(code->valuedouble));
    s.valid = true;
    s.stale = false;
    s.source = DataSource::Live;

    cJSON_Delete(root);
    return Result<WeatherSummary>::ok(s);
}

}  // namespace nova
