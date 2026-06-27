// NovaPainel - ui/boot_screen.hpp
// Real LVGL Boot/splash screen (Fase 4/5, ADR-0017/0023). Faithful port of
// docs/design/lvgl_export_reference/screens/screen_boot.c: badge + product
// name + version + animated progress bar + status message + skip button.
//
// The reference drives its progress bar from a fake random-increment task;
// here it's driven by real elapsed time (lv_tick_get()) against
// `duration_ms` instead - not fabricated, just repurposed as a loading
// animation while the splash holds (real hardware bring-up already
// finished synchronously before this screen ever renders, so there's no
// real per-step progress left to report by the time the splash shows).
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

class BootScreen {
public:
    // "Pular" intention - app_main.cpp wires this to skip straight past the
    // splash (cheap in-memory StateStore::set_screen(), no NVS/Wi-Fi, so it's
    // fine to call directly off the lvgl_task unlike the wizard's actions).
    using SkipFn = std::function<void()>;

    explicit BootScreen(uint32_t duration_ms, SkipFn on_skip)
        : duration_ms_(duration_ms), on_skip_(std::move(on_skip)) {}

    const char* name() const { return "BootScreen"; }

    void render(const AppState& state);

private:
    void build();

    static void on_skip_clicked(lv_event_t* e);

    uint32_t duration_ms_;
    SkipFn   on_skip_;

    bool      built_{false};
    uint32_t  start_tick_{0};
    lv_obj_t* root_{nullptr};
    lv_obj_t* progress_bar_{nullptr};
    lv_obj_t* status_label_{nullptr};
};

}  // namespace nova
