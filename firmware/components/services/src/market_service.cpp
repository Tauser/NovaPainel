// NovaPainel - services/market_service.cpp
#include "market_service.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "MarketService";
}

bool MarketService::init() {
    ESP_LOGI(kTag, "using provider: %s (MVP: CoinGecko REST in a later phase)",
             provider_.name());
    return true;
}

void MarketService::tick(uint32_t now_ms) {
    if (!orchestrator_.can_request(DataDomain::MarketSummary)) return;

    auto result = provider_.fetch_summary();
    orchestrator_.note_request(DataDomain::MarketSummary, now_ms);

    if (!result) {
        ESP_LOGW(kTag, "fetch failed: %s", to_string(result.error().code));
        return;
    }
    MarketSummary summary = result.value();
    summary.last_update_ms = now_ms;
    store_.set_market(summary);
    ESP_LOGI(kTag, "market updated: BTC=$%.0f  USD/BRL=%.2f  24h=%+.1f%%",
             summary.btc_usd, summary.usd_brl, summary.btc_change_24h);
}

}  // namespace nova
