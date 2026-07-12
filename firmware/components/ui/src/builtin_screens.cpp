#include "builtin_screens.hpp"

#include "event_bus.hpp"
#include "setup_screen.hpp"
#include "strings_ptbr.hpp"

namespace nova {

lv_obj_t* build_boot_screen(lv_obj_t* parent);
void update_boot_screen(const AppState& state);
lv_obj_t* build_home_screen(lv_obj_t* parent);
void update_home_screen(const AppState& state);
lv_obj_t* build_placeholder_screen(lv_obj_t* parent);
void update_placeholder_screen(const AppState& state);
lv_obj_t* build_setup_screen(lv_obj_t* parent);
void update_setup_screen(const AppState& state);

bool register_builtin_screens(ScreenRegistry& registry) {
    const ScreenSpec boot{
        ScreenId::Boot,
        strings_ptbr::kBootTitle,
        ui_event_bit(EventType::SystemChanged) | ui_event_bit(EventType::ScreenChanged),
        &build_boot_screen,
        &update_boot_screen,
        nullptr,
        nullptr,
    };
    const ScreenSpec home{
        ScreenId::Home,
        strings_ptbr::kHomeTitle,
        ui_event_bit(EventType::ClockChanged) |
            ui_event_bit(EventType::MarketChanged) |
            ui_event_bit(EventType::WeatherChanged) |
            ui_event_bit(EventType::SystemChanged) |
            ui_event_bit(EventType::ScreenChanged),
        &build_home_screen,
        &update_home_screen,
        nullptr,
        nullptr,
    };
    const ScreenSpec placeholder{
        ScreenId::Placeholder,
        strings_ptbr::kPlaceholderTitle,
        ui_event_bit(EventType::ScreenChanged) | ui_event_bit(EventType::SystemChanged),
        &build_placeholder_screen,
        &update_placeholder_screen,
        nullptr,
        nullptr,
    };
    const ScreenSpec setup{
        ScreenId::Setup,
        strings_ptbr::kSetupTitle,
        ui_event_bit(EventType::SetupChanged) |
            ui_event_bit(EventType::ScreenChanged) |
            ui_event_bit(EventType::SystemChanged),
        &build_setup_screen,
        &update_setup_screen,
        nullptr,
        nullptr,
    };

    return registry.register_screen(boot) &&
           registry.register_screen(home) &&
           registry.register_screen(placeholder) &&
           registry.register_screen(setup);
}

}  // namespace nova
