// NovaPainel - ui/main_shell.hpp
// Persistent app chrome (Fase 4/5) - faithful port of
// docs/design/lvgl_export_reference/novapanel.c's np_init()/np_build_rail()/
// np_build_topbar(): left nav rail (62px), topbar (60px), content area and
// a screen-indicator dot strip. Owns the one "MAIN phase" LVGL screen -
// HomeScreen (and future screens) mount their widgets inside content()
// instead of owning their own top-level screen.
//
// Deviation from the reference (user decision, this session): the rail
// starts CLOSED. Topbar/content PUSH over (resize/reposition, like the
// reference's permanently-visible rail) when it opens, rather than the
// rail overlaying on top. Opened/closed by tapping the small edge tab (a
// vertical strip with a >/< arrow, always visible at the current rail
// edge - the discoverability hint the user asked for) or by swiping
// right/left anywhere on screen. There's no separate topbar menu icon -
// the edge tab is the only affordance, by design (one less redundant
// control).
//
// Scope decision (user, this session): only "Inicio" is a real screen, so
// only its rail icon is checked/active. The other 6 rail icons + the
// topbar's bell/tint/gear icon buttons are drawn for visual fidelity but
// are inert (no screen to navigate to yet) - wire them up as each screen
// (Agenda, Mercado dedicado, Casa, Alarmes, Clima dedicado, Timer,
// Notificacoes, Configuracoes) actually gets built.
//
// MUST be called only while holding the board's LVGL lock (IBoard::lock) -
// see app_main.cpp.
#pragma once

#include <functional>

#include "app_state.hpp"

struct _lv_obj_t;
typedef struct _lv_obj_t lv_obj_t;
struct _lv_event_t;
typedef struct _lv_event_t lv_event_t;

namespace nova {

class MainShell {
public:
    // The rail's 8th icon (System diagnostics, Fase 7) is the second
    // navigable one besides "Inicio" - mirrors WizardScreen::SubmitFn's
    // injection pattern. The other 6 stay inert per the header comment.
    using NavigateFn = std::function<void(ScreenId)>;

    explicit MainShell(NavigateFn on_navigate) : on_navigate_(std::move(on_navigate)) {}

    const char* name() const { return "MainShell"; }

    void render(const AppState& state);

    // Where HomeScreen (and future "MAIN phase" screens) build their
    // widgets. Only valid after the first render() call.
    lv_obj_t* content() const { return content_; }

private:
    void build();
    void open_rail();
    void close_rail();
    void update_edge_tab();

    static void on_menu_toggle_clicked(lv_event_t* e);
    static void on_gesture(lv_event_t* e);
    static void on_system_icon_clicked(lv_event_t* e);

    NavigateFn on_navigate_;

    bool      built_{false};
    bool      rail_open_{false};
    lv_obj_t* root_{nullptr};
    lv_obj_t* rail_{nullptr};
    lv_obj_t* topbar_{nullptr};
    lv_obj_t* col_{nullptr};
    lv_obj_t* content_{nullptr};
    lv_obj_t* edge_tab_{nullptr};
    lv_obj_t* edge_tab_icon_{nullptr};
    lv_obj_t* title_label_{nullptr};
    lv_obj_t* wifi_icon_btn_{nullptr};
    lv_obj_t* wifi_icon_{nullptr};
};

}  // namespace nova
