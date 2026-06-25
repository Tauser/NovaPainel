// NovaPainel - services/market_service.cpp
#include "market_service.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "MarketService";
}

namespace {
struct CachedBtc {
    double   btc_usd;
    double   btc_change_24h;
    uint32_t last_update_ms;
};
constexpr const char* kCacheKey = "market_btc";
constexpr uint16_t    kCacheSchemaVersion = 1;
}  // namespace

bool MarketService::init() {
    ESP_LOGI(kTag, "using provider: %s (MVP: CoinGecko REST in a later phase)",
             provider_.name());

    CachedBtc cached{};
    if (cache_.read(kCacheKey, kCacheSchemaVersion, &cached, sizeof(cached))) {
        MarketSummary summary{};
        summary.btc_usd = cached.btc_usd;
        summary.btc_change_24h = cached.btc_change_24h;
        summary.last_update_ms = cached.last_update_ms;
        summary.valid = true;
        summary.stale = true;
        summary.source = DataSource::Cache;
        store_.set_market(summary);
        ESP_LOGI(kTag, "seeded from cache (last_update_ms=%lu)",
                 static_cast<unsigned long>(cached.last_update_ms));
    }
    return true;
}

void MarketService::tick(uint32_t now_ms) {
    // See WeatherService::tick() - same fix, same reasoning: wait for an
    // actual STA+IP connection before spending the request budget.
    if (store_.state().onboarding.wifi_status != WifiConnectStatus::Connected) return;
    if (!orchestrator_.can_request(DataDomain::MarketSummary)) return;

    auto result = provider_.fetch_summary();
    orchestrator_.note_request(DataDomain::MarketSummary, now_ms, result.is_ok());

    if (!result) {
        ESP_LOGW(kTag, "fetch failed: %s", to_string(result.error().code));
        return;
    }
    MarketSummary summary = result.value();
    summary.last_update_ms = now_ms;
    store_.set_market(summary);

    const CachedBtc cached{summary.btc_usd, summary.btc_change_24h, summary.last_update_ms};
    cache_.write(kCacheKey, kCacheSchemaVersion, &cached, sizeof(cached));

    ESP_LOGI(kTag, "market updated: BTC=$%.0f  24h=%+.1f%%",
             summary.btc_usd, summary.btc_change_24h);
}

}  // namespace nova
