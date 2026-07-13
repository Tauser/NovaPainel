#pragma once

#include "screen_registry.hpp"
#include "state_store.hpp"

#include <array>
#include <cstddef>

struct _lv_event_t;
typedef struct _lv_event_t lv_event_t;

namespace nova {

class UiShell {
public:
    UiShell(StateStore& state_store, const ScreenRegistry& screen_registry);

    void render(uint32_t pending_mask);
    void navigate(ScreenId screen_id);

private:
    struct NavTarget {
        UiShell* shell{nullptr};
        ScreenId screen_id{ScreenId::Boot};
    };

    struct ScreenSlot {
        ScreenId id{ScreenId::Boot};
        lv_obj_t* root{nullptr};
    };

    void build_shell();
    void ensure_screen_built(const ScreenSpec& spec);
    void switch_screen(const ScreenSpec& spec);
    void update_chrome(const ScreenSpec& spec, const AppState& state);
    void update_dots(ScreenId active_screen);
    void set_chrome_hidden(bool hidden);
    bool is_slider_screen(ScreenId screen_id) const;
    std::size_t slider_index_for(ScreenId screen_id) const;
    void navigate_by_delta(int delta);
    void update_toast();
    void show_toast(const char* text);
    void hide_toast();
    static void on_nav_click(lv_event_t* event);
    static void on_content_gesture(lv_event_t* event);

    StateStore& state_store_;
    const ScreenRegistry& screen_registry_;
    bool built_{false};
    bool has_active_screen_{false};
    ScreenId active_screen_{ScreenId::Boot};
    lv_obj_t* root_{nullptr};
    lv_obj_t* rail_{nullptr};
    lv_obj_t* topbar_{nullptr};
    lv_obj_t* topbar_title_{nullptr};
    lv_obj_t* content_{nullptr};
    lv_obj_t* dots_row_{nullptr};
    lv_obj_t* topbar_status_label_{nullptr};
    lv_obj_t* toast_panel_{nullptr};
    lv_obj_t* toast_label_{nullptr};
    lv_obj_t* keyboard_panel_{nullptr};
    std::array<lv_obj_t*, 12> dots_{};
    std::array<lv_obj_t*, 12> nav_buttons_{};
    std::array<lv_obj_t*, 12> nav_labels_{};
    std::array<ScreenId, 12> slider_screens_{};
    std::size_t slider_screen_count_{0};
    std::array<NavTarget, 12> nav_targets_{};
    std::array<ScreenSlot, 12> built_screens_{};
    bool toast_visible_{false};
};

}  // namespace nova
