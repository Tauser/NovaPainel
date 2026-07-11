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
    void update_chrome(const ScreenSpec& spec);
    void update_dots(ScreenId active_screen);
    void set_boot_mode(bool boot_mode);
    static void on_nav_click(lv_event_t* event);

    StateStore& state_store_;
    const ScreenRegistry& screen_registry_;
    bool built_{false};
    bool has_active_screen_{false};
    ScreenId active_screen_{ScreenId::Boot};
    lv_obj_t* root_{nullptr};
    lv_obj_t* rail_{nullptr};
    lv_obj_t* topbar_title_{nullptr};
    lv_obj_t* content_{nullptr};
    lv_obj_t* toast_label_{nullptr};
    std::array<lv_obj_t*, 12> dots_{};
    std::array<NavTarget, 12> nav_targets_{};
    std::array<ScreenSlot, 12> built_screens_{};
};

}  // namespace nova
