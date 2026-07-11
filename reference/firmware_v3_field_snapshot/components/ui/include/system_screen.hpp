// NovaPainel - ui/system_screen.hpp
// Real LVGL System/diagnostics screen (Fase 7, ADR-0028) - reset reason,
// reboot count, hardware-readiness flags and per-domain data age. Plain
// text MVP, no design reference exists for this screen yet (user decision).
//
// Owns its own top-level LVGL screen (like BootScreen/WizardScreen), NOT
// mounted inside MainShell::content() - MainShell's content area today
// assumes a single "MAIN phase" screen (Home) built once and never torn
// down; this avoids teaching it to swap screens for one MVP diagnostics
// view. Reached via the rail's 8th icon (MainShell), left via "Voltar".
//
// MUST be called only while holding the board's LVGL lock (IBoard::lock) -
// see app_main.cpp. render() builds the widget tree once on first call, then
// only updates label text on subsequent calls (no per-frame object churn).
#pragma once

#include <cstdint>
#include <functional>

#include "app_state.hpp"

struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;
struct _lv_event_t;
typedef struct _lv_event_t lv_event_t;

namespace nova {

class SystemScreen {
public:
    using BackFn = std::function<void()>;

    explicit SystemScreen(BackFn on_back) : on_back_(std::move(on_back)) {}

    const char* name() const { return "SystemScreen"; }

    void render(const AppState& state, uint32_t now_ms);

private:
    void build();

    static void on_back_clicked(lv_event_t* e);

    BackFn on_back_;

    bool      built_{false};
    lv_obj_t* root_{nullptr};
    lv_obj_t* reset_reason_label_{nullptr};
    lv_obj_t* reboot_count_label_{nullptr};
    lv_obj_t* hardware_label_{nullptr};
    lv_obj_t* weather_age_label_{nullptr};
    lv_obj_t* btc_age_label_{nullptr};
    lv_obj_t* usd_brl_age_label_{nullptr};
};

}  // namespace nova
