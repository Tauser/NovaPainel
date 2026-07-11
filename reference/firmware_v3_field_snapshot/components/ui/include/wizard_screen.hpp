// NovaPainel - ui/wizard_screen.hpp
// First-boot onboarding wizard (Fase 5, ADR-0017/0023). Faithful port of
// docs/design/lvgl_export_reference/screens/screen_setup.c (v2): 4 steps -
// Nome -> Wi-Fi -> Fuso+Formato -> Confirmacao - with a persistent
// progress bar / header / footer (Voltar, Continuar) and all 4 step panels
// built once, shown/hidden in place (not destroyed/rebuilt per step, unlike
// the v1 port of this screen).
//
// ADR-0017 rule: the UI never persists or calls hardware directly. Each step
// accumulates its choice in LOCAL UI state (selected_ssid_, etc.) and only
// calls `on_submit_` (injected via the constructor, same DI pattern as
// UiDispatcher::RenderFn) when "Continuar" is pressed - app_main.cpp wires
// that to StateStore::submit_onboarding(), the only path into SetupService.
// "Voltar" is pure local navigation (no persistence), so it never touches
// StateStore. The wizard only ever reads AppState::onboarding/preferences
// back, same as HomeScreen/BootScreen read AppState.
//
// MUST be called only while holding the board's LVGL lock (IBoard::lock) -
// see app_main.cpp.
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

class WizardScreen {
public:
    using SubmitFn = std::function<void(const OnboardingSubmission&)>;
    // Wifi step's "intention" to scan - same pattern as SubmitFn, just no
    // payload. app_main.cpp wires this to StateStore::request_wifi_scan().
    using RequestScanFn = std::function<void()>;

    WizardScreen(SubmitFn on_submit, RequestScanFn on_request_scan)
        : on_submit_(std::move(on_submit)),
          on_request_scan_(std::move(on_request_scan)) {}

    const char* name() const { return "WizardScreen"; }

    void render(const AppState& state);

    static constexpr int kTzCount = 5;        // matches kTimezoneList in the .cpp
    static constexpr int kFormatCount = 2;    // 24h, 12h
    static constexpr int kMaxWifiRows = 24;   // matches SetupService's scan cap

private:

    void build();
    void show_keyboard_for(lv_obj_t* textarea);
    void refresh_chrome();
    void go_to_step(OnboardingStep step);
    // Updates wifi_rows_ contents/visibility from wifi_networks_cache_ - the
    // only place that touches row text, called off the touch path (from
    // render(), when real scan data arrives). NOT called on selection.
    void refresh_wifi_network_list();
    // Just recolors/re-checkmarks the already-built wifi_rows_ - no object
    // creation, safe to call from a touch event callback (taskLVGL's stack
    // overflowed when this used to clean+rebuild the whole list on every
    // tap - see ADR-0025).
    void update_wifi_selection_ui();
    void update_tz_selection_ui();
    void update_format_selection_ui();
    void populate_confirmation();
    void submit_current_step();

    void build_name_panel(lv_obj_t* body);
    void build_wifi_panel(lv_obj_t* body);
    void build_tz_panel(lv_obj_t* body);
    void build_confirmation_panel(lv_obj_t* body);

    static void on_back_clicked(lv_event_t* e);
    static void on_next_clicked(lv_event_t* e);
    static void on_wifi_network_clicked(lv_event_t* e);
    static void on_wifi_rescan(lv_event_t* e);
    static void on_tz_row_clicked(lv_event_t* e);
    static void on_format_clicked(lv_event_t* e);
    static void on_textarea_focused(lv_event_t* e);
    static void on_kb_ready(lv_event_t* e);
    static void on_kb_cancel(lv_event_t* e);

    SubmitFn      on_submit_;
    RequestScanFn on_request_scan_;

    bool           built_{false};
    OnboardingStep current_step_{OnboardingStep::DisplayName};

    // Wifi step local state (not submitted until Continuar).
    std::string       selected_ssid_;
    WifiScanStatus     shown_wifi_scan_status_{WifiScanStatus::Idle};
    size_t             shown_wifi_network_count_{0};
    WifiScanStatus     wifi_scan_status_cache_{WifiScanStatus::Idle};
    std::vector<WifiNetwork> wifi_networks_cache_{};
    WifiConnectStatus  shown_wifi_status_{WifiConnectStatus::Idle};

    // Timezone+format step local state (not submitted until Continuar).
    int  selected_tz_index_{0};
    bool selected_format_24h_{true};

    lv_obj_t* root_{nullptr};
    lv_obj_t* keyboard_{nullptr};

    // -- chrome (built once, always present) --
    lv_obj_t* progress_bar_{nullptr};
    lv_obj_t* step_num_label_{nullptr};
    lv_obj_t* step_title_label_{nullptr};
    lv_obj_t* step_caption_label_{nullptr};
    lv_obj_t* back_btn_{nullptr};
    lv_obj_t* next_btn_{nullptr};
    lv_obj_t* next_label_{nullptr};
    lv_obj_t* status_label_{nullptr};  // Wifi connect feedback ("Conectando...", etc.)

    // -- step panels (built once, shown/hidden) --
    lv_obj_t* panel_name_{nullptr};
    lv_obj_t* panel_wifi_{nullptr};
    lv_obj_t* panel_tz_{nullptr};
    lv_obj_t* panel_confirm_{nullptr};

    // -- Nome panel --
    lv_obj_t* name_input_{nullptr};

    // -- Wifi panel -- rows are pre-built once (build_wifi_panel()), up to
    // kMaxWifiRows, then just shown/hidden/recolored as scan data and the
    // user's selection change - never destroyed/recreated (see methods above).
    lv_obj_t* wifi_list_{nullptr};
    lv_obj_t* wifi_status_text_{nullptr};  // "Buscando redes...", "Nenhuma encontrada", etc.
    lv_obj_t* wifi_rows_[kMaxWifiRows]{};
    lv_obj_t* wifi_row_checks_[kMaxWifiRows]{};
    int       wifi_row_count_{0};  // how many of wifi_rows_ are currently populated/visible
    lv_obj_t* wifi_ssid_label_{nullptr};      // "REDE SELECIONADA" value, in the side panel
    lv_obj_t* password_input_{nullptr};

    // -- Timezone+Format panel --
    lv_obj_t* tz_rows_[kTzCount]{};
    lv_obj_t* tz_checks_[kTzCount]{};
    lv_obj_t* format_boxes_[kFormatCount]{};
    lv_obj_t* format_checks_[kFormatCount]{};
    lv_obj_t* tz_selected_label_{nullptr};    // "FUSO SELECIONADO" value, in the side panel

    // -- Confirmation panel --
    lv_obj_t* confirm_name_label_{nullptr};
    lv_obj_t* confirm_wifi_label_{nullptr};
    lv_obj_t* confirm_tz_label_{nullptr};
    lv_obj_t* confirm_fmt_label_{nullptr};
};

}  // namespace nova
