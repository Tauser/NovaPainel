// NovaPainel - ui/settings_screen.hpp
// Post-onboarding settings screen (Fase 12). Own LVGL root screen.
// Faithful port of docs/design/lvgl_export_reference/screens/screen_config.c.
// COL 1: Rede (Wi-Fi card + BT toggle + fuso horário card).
// COL 2: Display & Som (sliders brilho/volume + modo noturno + formato pills).
// COL 3: Sistema (info table + update/restart buttons).
// ADR-0017: changes submitted via on_submit_ → StateStore → SetupService.
// MUST be called only while holding the board's LVGL lock.
#pragma once

#include <functional>

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

    static constexpr int kTzCount = 5;
    static constexpr int kSysRows = 7;

    SettingsScreen(SubmitFn on_submit, NavigateFn on_navigate,
                   RestartFn on_restart = {})
        : on_submit_(std::move(on_submit)),
          on_navigate_(std::move(on_navigate)),
          on_restart_(std::move(on_restart)) {}

    const char* name() const { return "SettingsScreen"; }
    void render(const AppState& state);

private:
    void build();
    void prefill(const AppState& state);
    void update_live(const AppState& state);
    void update_tz_card();
    void update_fmt_pills();
    void submit_tz_format();

    static void on_close_clicked(lv_event_t* e);
    static void on_wifi_reconfigure_clicked(lv_event_t* e);
    static void on_tz_card_clicked(lv_event_t* e);
    static void on_fmt24_clicked(lv_event_t* e);
    static void on_fmt12_clicked(lv_event_t* e);
    static void on_night_toggle_changed(lv_event_t* e);
    static void on_brilho_changed(lv_event_t* e);
    static void on_vsys_changed(lv_event_t* e);
    static void on_vmus_changed(lv_event_t* e);
    static void on_valm_changed(lv_event_t* e);
    static void on_restart_clicked(lv_event_t* e);

    SubmitFn   on_submit_;
    NavigateFn on_navigate_;
    RestartFn  on_restart_;

    bool built_{false};
    bool prefilled_{false};
    int  selected_tz_{0};
    bool selected_24h_{true};
    bool night_mode_{false};
    int  brilho_{80};
    int  vol_sys_{50};
    int  vol_mus_{40};
    int  vol_alm_{70};

    lv_obj_t* root_{nullptr};

    // Profile bar
    lv_obj_t* av_initials_lbl_{nullptr};
    lv_obj_t* profile_name_lbl_{nullptr};

    // Col 1 – Rede
    lv_obj_t* wifi_ssid_lbl_{nullptr};
    lv_obj_t* wifi_details_lbl_{nullptr};
    lv_obj_t* tz_name_lbl_{nullptr};
    lv_obj_t* tz_info_lbl_{nullptr};

    // Col 2 – Display & Som
    lv_obj_t* brilho_val_lbl_{nullptr};
    lv_obj_t* vsys_val_lbl_{nullptr};
    lv_obj_t* vmus_val_lbl_{nullptr};
    lv_obj_t* valm_val_lbl_{nullptr};
    lv_obj_t* fmt_pills_[2]{};
    lv_obj_t* night_sw_{nullptr};

    // Col 3 – Sistema
    lv_obj_t* sys_vals_[kSysRows]{};
};

}  // namespace nova
