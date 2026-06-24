// NovaPainel - ui/home_screen.hpp
// Real LVGL Home screen (Fase 4). Layout/colors follow the design reference
// in docs/design/lvgl_export_reference/ (screen_home.c, novapanel_theme.h),
// but scoped to what AppState actually has today: clock + market snapshot +
// system status. No weather/agenda/scenes/player widgets - those need
// providers/services that don't exist yet (see docs/design/README.md for the
// fase mapping of each reference screen).
//
// MUST be called only while holding the board's LVGL lock (IBoard::lock) -
// see app_main.cpp. render() builds the widget tree once on first call, then
// only updates label text on subsequent calls (no per-frame object churn).
#pragma once

#include "app_state.hpp"

struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;

namespace nova {

class HomeScreen {
public:
    const char* name() const { return "HomeScreen"; }

    void render(const AppState& state);

private:
    void build();

    bool     built_{false};
    lv_obj_t* clock_label_{nullptr};
    lv_obj_t* seconds_label_{nullptr};
    lv_obj_t* date_label_{nullptr};
    lv_obj_t* btc_label_{nullptr};
    lv_obj_t* btc_change_label_{nullptr};
    lv_obj_t* usd_brl_label_{nullptr};
    lv_obj_t* status_label_{nullptr};
};

}  // namespace nova
