// NovaPainel - ui/settings_screen.hpp
// Post-onboarding settings screen (Fase 12). Own LVGL root screen.
// Faithful port of docs/design/lvgl_export_reference/screens/screen_config.c.
// COL 1: Rede (Wi-Fi card + BT toggle + fuso horário card).
// COL 2: Display & Som (sliders brilho/volume + modo noturno + formato pills).
// COL 3: Sistema (info table + update/restart buttons).
// Wi-Fi "Gerenciar" opens a modal (lv_layer_top) — NOT the Setup wizard.
// Password keyboard uses a SEPARATE full-screen bottom panel on lv_layer_top()
// so it spans 100% width; the textarea sits in the toolbar above the keyboard.
// ADR-0017: changes submitted via on_submit_ → StateStore → SetupService.
// MUST be called only while holding the board's LVGL lock.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "app_state.hpp"

struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;
struct _lv_event_t;
typedef struct _lv_event_t lv_event_t;

namespace nova {

class NovaKeyboardManager;

class SettingsScreen {
public:
    using SubmitFn   = std::function<void(const OnboardingSubmission&)>;
    using NavigateFn = std::function<void(ScreenId)>;
    using ScanFn     = std::function<void()>;
    using RestartFn  = std::function<void()>;

    static constexpr int kTzCount = 5;
    static constexpr int kSysRows = 7;

    SettingsScreen(SubmitFn on_submit, NavigateFn on_navigate,
                   ScanFn on_scan = {}, RestartFn on_restart = {})
        : on_submit_(std::move(on_submit)),
          on_navigate_(std::move(on_navigate)),
          on_scan_(std::move(on_scan)),
          on_restart_(std::move(on_restart)) {}

    ~SettingsScreen();

    const char* name() const { return "SettingsScreen"; }
    void render(const AppState& state);

private:
    // ── Main screen ───────────────────────────────────────────────────────
    void build();
    void prefill(const AppState& state);
    void update_live(const AppState& state);
    void update_tz_card();
    void update_fmt_pills();
    void submit_tz_format();

    static void on_close_clicked(lv_event_t* e);
    static void on_wifi_manage_clicked(lv_event_t* e);
    static void on_tz_card_clicked(lv_event_t* e);
    static void on_fmt24_clicked(lv_event_t* e);
    static void on_fmt12_clicked(lv_event_t* e);
    static void on_night_toggle_changed(lv_event_t* e);
    static void on_brilho_changed(lv_event_t* e);
    static void on_vsys_changed(lv_event_t* e);
    static void on_vmus_changed(lv_event_t* e);
    static void on_valm_changed(lv_event_t* e);
    static void on_restart_clicked(lv_event_t* e);

    // ── Wi-Fi modal (network list) ────────────────────────────────────────
    void open_wifi_modal(const AppState& state);
    void close_wifi_modal();
    void rebuild_modal_nets(const AppState& state);

    static void on_modal_close(lv_event_t* e);
    static void on_modal_scan(lv_event_t* e);
    static void on_modal_disconnect(lv_event_t* e);
    static void on_modal_net_clicked(lv_event_t* e);

    // ── Keyboard panel (bottom, full-width, separate lv_layer_top child) ─
    void show_kb_panel(const std::string& ssid, bool secured);
    void hide_kb_panel();

    static void on_kbpanel_connect(lv_event_t* e);
    static void on_kbpanel_close(lv_event_t* e);
    static void on_kb_ready(lv_event_t* e);
    static void on_kb_cancel(lv_event_t* e);

    // ── Members ───────────────────────────────────────────────────────────
    SubmitFn   on_submit_;
    NavigateFn on_navigate_;
    ScanFn     on_scan_;
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

    // Main screen widgets
    lv_obj_t* root_{nullptr};
    lv_obj_t* av_initials_lbl_{nullptr};
    lv_obj_t* profile_name_lbl_{nullptr};
    lv_obj_t* wifi_ssid_lbl_{nullptr};
    lv_obj_t* wifi_details_lbl_{nullptr};
    lv_obj_t* tz_name_lbl_{nullptr};
    lv_obj_t* tz_info_lbl_{nullptr};
    lv_obj_t* brilho_val_lbl_{nullptr};
    lv_obj_t* vsys_val_lbl_{nullptr};
    lv_obj_t* vmus_val_lbl_{nullptr};
    lv_obj_t* valm_val_lbl_{nullptr};
    lv_obj_t* fmt_pills_[2]{};
    lv_obj_t* night_sw_{nullptr};
    lv_obj_t* sys_vals_[kSysRows]{};

    // Wi-Fi modal (network list overlay)
    bool        modal_open_{false};
    std::string modal_sel_ssid_;
    bool        modal_sel_secured_{false};
    std::vector<std::string> modal_ssids_;
    WifiScanStatus last_scan_status_{WifiScanStatus::Idle};

    lv_obj_t* modal_overlay_{nullptr};       // lv_layer_top() child — dims + box
    lv_obj_t* modal_scan_lbl_{nullptr};      // "Buscando..." / "X redes"
    lv_obj_t* modal_connected_sec_{nullptr}; // section shown when online
    lv_obj_t* modal_connected_lbl_{nullptr}; // SSID of connected network
    lv_obj_t* modal_net_list_{nullptr};      // flex col of network rows

    // Wi-Fi modal dialog box (child of modal_overlay_)
    lv_obj_t* modal_box_{nullptr};
    // Password input section (inside modal_box_, hidden until network selected)
    lv_obj_t* modal_password_box_{nullptr};
    lv_obj_t* modal_password_ssid_lbl_{nullptr};
    lv_obj_t* modal_password_ta_{nullptr};
    // Shared bottom keyboard — created in build(), deleted in destructor
    NovaKeyboardManager* keyboard_manager_{nullptr};
};

}  // namespace nova
