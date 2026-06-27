#pragma once

#include <cstddef>
#include <functional>

#include "app_state.hpp"
#include "lvgl.h"
#include "novapanel_theme.hpp"

namespace nova {

inline constexpr std::size_t kUiScreenCount =
    static_cast<std::size_t>(ScreenId::Weather) + 1;

extern lv_obj_t *np_root;
extern lv_obj_t *np_rail;
extern lv_obj_t *np_topbar;
extern lv_obj_t *np_content;
extern lv_obj_t *np_dots_bar;
extern lv_obj_t *np_screens[kUiScreenCount];

using BootSkipFn = std::function<void()>;
using SetupSubmitFn = std::function<void(const OnboardingSubmission&)>;
using SetupScanFn = std::function<void()>;
using SetupStepFn = std::function<void(OnboardingStep)>;
using NotificationsOpenFn = std::function<void()>;

void np_init();
void np_bind_boot_skip(BootSkipFn fn);
void np_bind_setup_submit(SetupSubmitFn fn);
void np_bind_setup_scan(SetupScanFn fn);
void np_bind_setup_step(SetupStepFn fn);
void np_bind_notifications_open(NotificationsOpenFn fn);
void np_update_boot(const BootState& boot);
void np_update_setup(const AppState& state);
void np_update_home(const AppState& state);
void np_update_market(const AppState& state);
void np_update_settings(const AppState& state);
void np_update_weather(const AppState& state);
void np_update_shell(const AppState& state);
void np_tick_setup();
void np_navigate_to(ScreenId screen);
void np_tick();

const char *np_screen_title(ScreenId screen);

void np_screen_boot(lv_obj_t *parent);
void np_screen_setup(lv_obj_t *parent);
void np_screen_home(lv_obj_t *parent);
void np_screen_calendar(lv_obj_t *parent);
void np_screen_market(lv_obj_t *parent);
void np_screen_devices(lv_obj_t *parent);
void np_screen_focus(lv_obj_t *parent);
void np_screen_photoframe(lv_obj_t *parent);
void np_screen_routines(lv_obj_t *parent);
void np_screen_settings(lv_obj_t *parent);
void np_screen_weather(lv_obj_t *parent);

}  // namespace nova
