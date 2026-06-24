// NovaPainel - ui/src/screens/home_screen.cpp
#include "home_screen.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "HomeScreen";

const char* weekday_name(int wd) {
    static const char* kDays[] = {"Domingo", "Segunda", "Terca", "Quarta",
                                  "Quinta", "Sexta", "Sabado"};
    if (wd < 0 || wd > 6) return "?";
    return kDays[wd];
}
}  // namespace

void HomeScreen::render(const AppState& s) {
    // This block stands in for the LVGL widget tree. Layout/logic identical to
    // the planned Home card; only the draw calls are logs for now.
    const auto& c = s.clock;
    const auto& m = s.market;
    const auto& sys = s.system;

    ESP_LOGI(kTag, "+------------------ HOME (mock render) ------------------+");
    ESP_LOGI(kTag, "| %02d:%02d:%02d   %s, %02d/%02d/%04d%s",
             c.hour, c.minute, c.second, weekday_name(c.weekday), c.day,
             c.month, c.year, c.synced ? "" : "   [hora nao sincronizada]");
    if (m.valid) {
        const char* src = (m.source == DataSource::Cache) ? "  [cache]"
                        : (m.source == DataSource::Mock)  ? "  [mock]"
                        : "";
        ESP_LOGI(kTag, "| BTC: $%.0f  (%+.1f%% 24h)%s", m.btc_usd,
                 m.btc_change_24h, src);
        ESP_LOGI(kTag, "| USD/BRL: R$ %.2f", m.usd_brl);
    } else {
        ESP_LOGI(kTag, "| Mercado: sem dados ainda");
    }
    ESP_LOGI(kTag, "| Sistema: board=%d display=%d touch=%d net=%d sd=%d cache=%d",
             sys.board_ready, sys.display_ready, sys.touch_ready,
             sys.network_ready, sys.sd_ready, sys.cache_ready);
    ESP_LOGI(kTag, "+--------------------------------------------------------+");
}

}  // namespace nova
