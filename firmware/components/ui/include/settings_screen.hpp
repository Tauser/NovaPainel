// NovaPainel - ui/settings_screen.hpp
// Post-onboarding settings screen (Fase 12). Own LVGL root screen (same
// pattern as WizardScreen/SystemScreen). Port of
// docs/design/lvgl_export_reference/screens/screen_config.c.
// ADR-0017: changes are submitted via on_submit_ → StateStore →
// SetupService (never written directly from the UI).
// MUST be called only while holding the board's LVGL lock.
#pragma once

#include <functional>
#include <string>

#include "app_state.hpp"

struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;
struct _lv_event_t;
typedef struct _lv_event_t lv_event_t;

namespace nova {

class SettingsScreen {
public:
    using SubmitFn   = std::function<void(const OnboardingSubmission&)>;
    using NavigateFn = std::function<void(ScreenId)>;
    using RestartFn  = std::function<void()>;

    SettingsScreen(SubmitFn on_submit, NavigateFn on_navigate,
                   RestartFn on_restart = {})
        : on_submit_(std::move(on_submit)),
          on_navigate_(std::move(on_navigate)),
          on_restart_(std::move(on_restart)) {}

    const char* name() const { return "SettingsScreen"; }
    void render(const AppState& state);

    static constexpr int kTzCount  = 5;
    static constexpr int kFmtCount = 2;

private:
    void build();
    void prefill(const AppState& state);
    void update_tz_ui();
    void update_format_ui();
    void update_live(const AppState& state);
    void show_keyboard_for(lv_obj_t* ta);
    void submit_name();
    void submit_tz_format();

    static void on_close_clicked(lv_event_t* e);
    static void on_edit_name_clicked(lv_event_t* e);
    static void on_tz_clicked(lv_event_t* e);
    static void on_format_clicked(lv_event_t* e);
    static void on_wifi_reconfigure_clicked(lv_event_t* e);
    static void on_textarea_focused(lv_event_t* e);
    static void on_kb_ready(lv_event_t* e);
    static void on_kb_cancel(lv_event_t* e);
    static void on_restart_clicked(lv_event_t* e);

    SubmitFn   on_submit_;
    NavigateFn on_navigate_;
    RestartFn  on_restart_;

    bool built_{false};
    bool prefilled_{false};
    int  selected_tz_{0};
    bool selected_24h_{true};

    lv_obj_t* root_{nullptr};
    lv_obj_t* keyboard_{nullptr};
    lv_obj_t* name_input_{nullptr};
    lv_obj_t* av_initials_label_{nullptr};
    lv_obj_t* profile_name_label_{nullptr};
    lv_obj_t* wifi_status_label_{nullptr};
    lv_obj_t* sys_chip_label_{nullptr};
    lv_obj_t* sys_reset_label_{nullptr};
    lv_obj_t* sys_reboots_label_{nullptr};
    lv_obj_t* sys_net_label_{nullptr};
    lv_obj_t* tz_rows_[kTzCount]{};
    lv_obj_t* tz_checks_[kTzCount]{};
    lv_obj_t* fmt_rows_[kFmtCount]{};
    lv_obj_t* fmt_checks_[kFmtCount]{};
};

}  // namespace nova
