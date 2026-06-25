// NovaPainel - ui/home_screen.hpp
// Real LVGL Home screen (Fase 4/5). Layout/colors/sizes follow
// docs/design/lvgl_export_reference/screens/screen_home.c +
// novapanel_theme.h as closely as the current AppState allows:
//   - Clock, weather (OpenMeteoProvider, incl. high/low/feels-like/UV from
//     its current+daily forecast) and market mini (CoinGeckoProvider, minus
//     an Ibovespa row - no such provider) use REAL data.
//   - Agenda/Cenas rápidas/Player have no backing service yet (no
//     CalendarService, no scenes/automation, no audio) - per user decision,
//     they're built with the reference's own sample placeholder content so
//     the screen matches the design now; swap to real data source-by-source
//     as each service lands, without touching layout.
//
// MainShell (ADR-0024) owns the actual lv_screen_load() - this mounts its
// widgets inside MainShell::content() instead of owning a top-level screen
// of its own. MUST be called only while holding the board's LVGL lock
// (IBoard::lock) - see app_main.cpp. render() builds the widget tree once
// on first call, then only updates label text on subsequent calls (no
// per-frame object churn).
#pragma once

#include "app_state.hpp"

struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;

namespace nova {

class HomeScreen {
public:
    const char* name() const { return "HomeScreen"; }

    // `content_parent` is MainShell::content() - only used on the first
    // call (where the widget tree is built into it).
    void render(const AppState& state, lv_obj_t* content_parent);

private:
    void build(lv_obj_t* parent);

    bool      built_{false};

    // -- clock --
    lv_obj_t* clock_label_{nullptr};
    lv_obj_t* seconds_label_{nullptr};
    lv_obj_t* date_label_{nullptr};

    // -- weather (real, OpenMeteoProvider) --
    lv_obj_t* weather_icon_label_{nullptr};
    lv_obj_t* weather_temp_label_{nullptr};
    lv_obj_t* weather_condition_label_{nullptr};
    lv_obj_t* weather_wind_label_{nullptr};
    lv_obj_t* weather_humidity_label_{nullptr};
    lv_obj_t* weather_highlow_label_{nullptr};
    lv_obj_t* weather_feels_label_{nullptr};
    lv_obj_t* weather_uv_label_{nullptr};

    // -- market mini, bottom-right (real, CoinGeckoProvider) --
    lv_obj_t* market_usd_label_{nullptr};
    lv_obj_t* market_usd_change_label_{nullptr};
    lv_obj_t* market_btc_label_{nullptr};
    lv_obj_t* market_btc_change_label_{nullptr};

    // Board/system diagnostics - our own addition, not in the reference
    // design, kept small/muted at the bottom of the clock card.
    lv_obj_t* status_label_{nullptr};
};

}  // namespace nova
