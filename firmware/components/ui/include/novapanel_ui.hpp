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
using NavigateRequestFn = std::function<void(ScreenId)>;
using OhlcPeriodFn         = std::function<void(OhlcPeriod)>;
// Settings callbacks — tipos simples para evitar static std::function globais.
// Cada callback envia só o campo que mudou; app_main lê as prefs atuais do
// StateStore, altera o campo e chama set_preferences().
using SettingsTimeFormatFn  = std::function<void(bool)>;       // true=24h
using SettingsThemeFn       = std::function<void(ThemeMode)>;
using SettingsRebootFn      = std::function<void()>;
using SettingsBrightnessFn        = std::function<void(int)>;  // RELEASED: aplica PWM + salva NVS
using SettingsBrightnessPreviewFn = std::function<void(int)>;  // VALUE_CHANGED: só PWM, sem NVS
using SettingsVolumeFn            = std::function<void(int)>;
// Chamado quando um valor de settings é confirmado (discrete change ou slider released).
// msg é uma string curta do tipo "Brilho: 75%" ou "Formato 24h salvo".
using SettingsNotifyFn      = std::function<void(const char* msg)>;

void np_init();
void np_bind_boot_skip(BootSkipFn fn);
void np_bind_setup_submit(SetupSubmitFn fn);
void np_bind_setup_scan(SetupScanFn fn);
void np_bind_setup_step(SetupStepFn fn);
void np_bind_notifications_open(NotificationsOpenFn fn);
// Liga a navegacao da UI ao StateStore (publica ScreenChanged), para a tela
// de destino ser atualizada com o estado atual na hora da troca, em vez de
// esperar o proximo evento periodico do dominio (ex.: Market a cada 60s).
void np_bind_navigate(NavigateRequestFn fn);
// Liga o seletor de período do gráfico ao MarketService.
void np_bind_ohlc_period(OhlcPeriodFn fn);
// Liga as ações interativas da tela de Settings.
void np_bind_settings_time_format(SettingsTimeFormatFn fn);
void np_bind_settings_theme(SettingsThemeFn fn);
void np_bind_settings_reboot(SettingsRebootFn fn);
void np_bind_settings_brightness(SettingsBrightnessFn fn);
void np_bind_settings_brightness_preview(SettingsBrightnessPreviewFn fn);
void np_bind_settings_volume(SettingsVolumeFn fn);
void np_bind_settings_notify(SettingsNotifyFn fn);
void np_update_boot(const BootState& boot);
void np_update_setup(const AppState& state);
void np_update_home(const AppState& state);
void np_update_market(const AppState& state, const OhlcSeries& ohlc);
void np_update_settings(const AppState& state);
void np_update_weather(const AppState& state);
void np_update_shell(const AppState& state);
void np_navigate_to(ScreenId screen);
// Exibe toast sobre qualquer tela; desaparece automaticamente após ms milissegundos.
void np_show_toast(const char* title, const char* body,
                   NotificationLevel level = NotificationLevel::Info,
                   uint32_t duration_ms = 3000);
void np_tick_setup();
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
