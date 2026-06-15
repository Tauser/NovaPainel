// NovaPainel - ui/home_screen.hpp
// Log-only Home screen (NO LVGL yet - ADR-0007 / decisions 7-9). It reads the
// AppState and "renders" by printing what the real LVGL widgets will show:
// time, date, BTC, USD/BRL and system status. When LVGL lands, only render()'s
// body changes; it will still be driven from the lvgl_task via UiDispatcher.
#pragma once

#include "app_state.hpp"

namespace nova {

class HomeScreen {
public:
    const char* name() const { return "HomeScreen"; }

    // MUST be called on the (future) lvgl_task. Here: emits a log "frame".
    void render(const AppState& state);
};

}  // namespace nova
