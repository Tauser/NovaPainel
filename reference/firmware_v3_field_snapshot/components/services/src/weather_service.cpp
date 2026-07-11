// NovaPainel - services/weather_service.cpp
#include "weather_service.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "WeatherService";
}

namespace {
struct CachedWeather {
    double           temperature_c;
    double           feels_like_c;
    double           temp_max_c;
    double           temp_min_c;
    double           uv_index;
    double           wind_speed_kmh;
    int              humidity_pct;
    WeatherCondition condition;
    uint32_t         last_update_ms;
};
constexpr const char*  kCacheKey = "weather";
constexpr uint16_t     kCacheSchemaVersion = 1;
}  // namespace

bool WeatherService::init() {
    ESP_LOGI(kTag, "using provider: %s", provider_.name());

    CachedWeather cached{};
    if (cache_.read(kCacheKey, kCacheSchemaVersion, &cached, sizeof(cached))) {
        WeatherSummary summary{};
        summary.temperature_c = cached.temperature_c;
        summary.feels_like_c = cached.feels_like_c;
        summary.temp_max_c = cached.temp_max_c;
        summary.temp_min_c = cached.temp_min_c;
        summary.uv_index = cached.uv_index;
        summary.wind_speed_kmh = cached.wind_speed_kmh;
        summary.humidity_pct = cached.humidity_pct;
        summary.condition = cached.condition;
        summary.last_update_ms = cached.last_update_ms;
        summary.valid = true;
        summary.stale = true;
        summary.source = DataSource::Cache;
        store_.set_weather(summary);
        ESP_LOGI(kTag, "seeded from cache (last_update_ms=%lu)",
                 static_cast<unsigned long>(cached.last_update_ms));
    }
    return true;
}

void WeatherService::tick(uint32_t now_ms) {
    // Wait for an actual STA+IP connection (SetupService's
    // onboarding.wifi_status, updated on IP_EVENT_STA_GOT_IP/disconnect) -
    // SystemStatus::network_ready only means the ESP-Hosted/SDIO transport is
    // up (set once at boot), not that Wi-Fi has associated. Without this
    // check, the very first tick after boot fired the HTTPS request before
    // esp_wifi_connect() (kicked off async by SetupService::init()) had any
    // chance to associate, wasting the request instead of waiting for it.
    if (store_.state().onboarding.wifi_status != WifiConnectStatus::Connected) return;
    if (!orchestrator_.can_request(DataDomain::Weather)) return;

    auto result = provider_.fetch_current();
    orchestrator_.note_request(DataDomain::Weather, now_ms, result.is_ok());

    if (!result) {
        ESP_LOGW(kTag, "fetch failed: %s", to_string(result.error().code));
        return;
    }
    WeatherSummary summary = result.value();
    summary.last_update_ms = now_ms;
    store_.set_weather(summary);

    const CachedWeather cached{summary.temperature_c,  summary.feels_like_c,
                               summary.temp_max_c,      summary.temp_min_c,
                               summary.uv_index,        summary.wind_speed_kmh,
                               summary.humidity_pct,    summary.condition,
                               summary.last_update_ms};
    cache_.write(kCacheKey, kCacheSchemaVersion, &cached, sizeof(cached));

    ESP_LOGI(kTag, "weather updated: %.1fC humidity=%d%% wind=%.1fkm/h condition=%d",
             summary.temperature_c, summary.humidity_pct, summary.wind_speed_kmh,
             static_cast<int>(summary.condition));
}

}  // namespace nova
