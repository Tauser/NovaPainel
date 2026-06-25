// NovaPainel - services/forex_service.cpp
#include "forex_service.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "ForexService";
}

namespace {
struct CachedForex {
    double   usd_brl;
    uint32_t last_update_ms;
};
constexpr const char* kCacheKey = "market_forex";
constexpr uint16_t    kCacheSchemaVersion = 1;
}  // namespace

bool ForexService::init() {
    ESP_LOGI(kTag, "using provider: %s", provider_.name());

    CachedForex cached{};
    if (cache_.read(kCacheKey, kCacheSchemaVersion, &cached, sizeof(cached))) {
        store_.set_usd_brl_rate(cached.usd_brl, DataSource::Cache, cached.last_update_ms);
        ESP_LOGI(kTag, "seeded from cache (last_update_ms=%lu)",
                 static_cast<unsigned long>(cached.last_update_ms));
    }
    return true;
}

void ForexService::tick(uint32_t now_ms) {
    // See MarketService::tick()/WeatherService::tick() - same fix, same
    // reasoning: wait for an actual STA+IP connection before spending the
    // request budget.
    if (store_.state().onboarding.wifi_status != WifiConnectStatus::Connected) return;
    if (!orchestrator_.can_request(DataDomain::Forex)) return;

    auto result = provider_.fetch_usd_brl();
    orchestrator_.note_request(DataDomain::Forex, now_ms, result.is_ok());

    if (!result) {
        ESP_LOGW(kTag, "fetch failed: %s", to_string(result.error().code));
        return;
    }
    store_.set_usd_brl_rate(result.value(), DataSource::Live, now_ms);

    const CachedForex cached{result.value(), now_ms};
    cache_.write(kCacheKey, kCacheSchemaVersion, &cached, sizeof(cached));

    ESP_LOGI(kTag, "USD/BRL updated: %.4f", result.value());
}

}  // namespace nova
