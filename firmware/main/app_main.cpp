#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <functional>

#include "bsp/esp32_p4_wifi6_touch_lcd_7b.h"
#include "event_bus.hpp"
#include "clock_service.hpp"
#include "bootstrap_service.hpp"
#include "notification_service.hpp"
#include "request_orchestrator.hpp"
#include "cache_store.hpp"
#include "market_service.hpp"
#include "forex_service.hpp"
#include "weather_service.hpp"
#include "network_worker.hpp"
#include "coingecko_provider.hpp"
#include "forex_provider.hpp"
#include "open_meteo_provider.hpp"
#include "novapanel_ui.hpp"
#include "setup_service.hpp"
#include "system_service.hpp"
#include "service_manager.hpp"
#include "state_store.hpp"
#include "ui_dispatcher.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#if __has_include("esp_wifi.h") && __has_include("esp_netif.h") && __has_include("esp_event.h")
#define NOVAPANEL_HAS_WIFI_TRANSPORT 1
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#else
#define NOVAPANEL_HAS_WIFI_TRANSPORT 0
#endif

using namespace nova;

static const char *TAG = "NovaPanel";

// Rotacao 180 por PPA (sw_rotate). Este painel EK79007 (DSI video-mode) NAO
// gira por mirror de hardware (MADCTL sem efeito visivel), entao a rotacao
// precisa ser por software/PPA. Por isso nao usamos avoid_tearing aqui.
#define NOVAPANEL_DISPLAY_ROTATION LV_DISPLAY_ROTATION_180
#define NOVAPANEL_LVGL_LOCK_MS 1000
#define NOVAPANEL_LVGL_MAX_TIMEOUTS 5

static void log_heap_snapshot(const char *phase)
{
    ESP_LOGI(TAG,
             "%s: uptime_ms=%" PRId64 " free_internal=%u min_internal=%u free_spiram=%u min_spiram=%u",
             phase,
             esp_timer_get_time() / 1000,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
}

static void start_display(void)
{
    ESP_LOGI(TAG, "starting Waveshare BSP display");

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        // Config de producao: double buffer em PSRAM + rotacao 180 por PPA.
        //
        // Display: MIPI DSI, EK79007 controller, 1024x600 @ 60 Hz.
        // sw_rotate=true usa o PPA do ESP32-P4 p/ girar 180 (o painel nao gira
        // por mirror de hardware). avoid_tearing NAO e usado porque e
        // incompativel com sw_rotate neste BSP, e o flicker observado e do
        // backlight (GPIO32), nao tearing.
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_V_RES,
        .double_buffer = true,
        .flags = {
            .buff_dma    = true,
            .buff_spiram = true,
            .sw_rotate   = true,
        },
    };
    cfg.lvgl_port_cfg.task_stack = 16384;

    lv_display_t *display = bsp_display_start_with_config(&cfg);
    if (display == NULL) {
        ESP_LOGE(TAG, "display start failed");
        abort();
    }

    bsp_display_rotate(display, NOVAPANEL_DISPLAY_ROTATION);
    // NOTA: NAO forcar RENDER_MODE_FULL. Em modo parcial o LVGL so redesenha
    // a regiao suja, reduzindo muito a banda de PSRAM gasta por update (PPA +
    // escrita no framebuffer). Isso deixa banda livre para o DSI ler o
    // framebuffer e evita o underrun (flash branco) quando a rede esta ativa.
    ESP_LOGI(TAG, "display rotation=%d sw_rotate=1(PPA) render=partial buf=%ux%u", (int)NOVAPANEL_DISPLAY_ROTATION, (unsigned)BSP_LCD_H_RES, (unsigned)BSP_LCD_V_RES);
}

static bool with_lvgl_lock(const char* phase, int& consecutive_failures,
                           const std::function<void()>& fn)
{
    if (!bsp_display_lock(NOVAPANEL_LVGL_LOCK_MS)) {
        ++consecutive_failures;
        ESP_LOGE(TAG, "LVGL lock timeout during %s (%d/%d)",
                 phase, consecutive_failures, NOVAPANEL_LVGL_MAX_TIMEOUTS);
        if (consecutive_failures >= NOVAPANEL_LVGL_MAX_TIMEOUTS) {
            ESP_LOGE(TAG, "LVGL remained unavailable after repeated timeouts; keeping system alive");
        }
        return false;
    }

    consecutive_failures = 0;
    fn();
    bsp_display_unlock();
    return true;
}

static bool start_network_transport(void)
{
#if NOVAPANEL_HAS_WIFI_TRANSPORT
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    if (nvs_err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(nvs_err));
        return false;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
        return false;
    }

    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "ESP-Hosted/SDIO transport requested for C6");
    return true;
#else
    ESP_LOGW(TAG, "network transport skipped in host build");
    return false;
#endif
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "app_main entry");
    log_heap_snapshot("app_main_entry");

    EventBus bus;
    ESP_LOGI(TAG, "boot trace: EventBus ok");
    StateStore state(bus);
    ESP_LOGI(TAG, "boot trace: StateStore ok");
    UiDispatcher ui(bus);
    ESP_LOGI(TAG, "boot trace: UiDispatcher ok");
    RequestOrchestrator requests;
    ESP_LOGI(TAG, "boot trace: RequestOrchestrator ok");
    CacheStore cache;
    ESP_LOGI(TAG, "boot trace: CacheStore ok");
    ServiceManager services;
    ESP_LOGI(TAG, "boot trace: ServiceManager ok");
    BootstrapService bootstrap(state, bus);
    ESP_LOGI(TAG, "boot trace: BootstrapService ok");
    SetupService setup(state, bus);
    ESP_LOGI(TAG, "boot trace: SetupService ok");
    SystemService system_service(state);
    ESP_LOGI(TAG, "boot trace: SystemService ok");
    NotificationService notification_service(state, bus);
    ESP_LOGI(TAG, "boot trace: NotificationService ok");
    ClockService clock_service(state);
    ESP_LOGI(TAG, "boot trace: ClockService ok");
    CoinGeckoProvider coingecko_provider;
    ESP_LOGI(TAG, "boot trace: CoinGeckoProvider ok");
    ForexProvider forex_provider;
    ESP_LOGI(TAG, "boot trace: ForexProvider ok");
    OpenMeteoProvider open_meteo_provider;
    ESP_LOGI(TAG, "boot trace: OpenMeteoProvider ok");
    MarketService market_service(state, requests, cache, coingecko_provider);
    ESP_LOGI(TAG, "boot trace: MarketService ok");
    ForexService forex_service(state, requests, cache, forex_provider);
    ESP_LOGI(TAG, "boot trace: ForexService ok");
    WeatherService weather_service(state, requests, cache, open_meteo_provider);
    ESP_LOGI(TAG, "boot trace: WeatherService ok");

    // Fase 2 (ADR-0035): uma task de rede unica dirige os refresh() por
    // prioridade, serializado e escalonado. Os services de dados nao tem mais
    // task propria; o worker chama o refresh de cada dominio "due".
    NetworkWorker network_worker(state, requests);
    ESP_LOGI(TAG, "boot trace: NetworkWorker ok");
    network_worker.register_fetcher(DataDomain::MarketSummary,
        [&market_service](uint32_t t) { return market_service.refresh(t); });
    network_worker.register_fetcher(DataDomain::Forex,
        [&forex_service](uint32_t t) { return forex_service.refresh(t); });
    network_worker.register_fetcher(DataDomain::Weather,
        [&weather_service](uint32_t t) { return weather_service.refresh(t); });
    ESP_LOGI(TAG, "boot trace: fetchers registered");

    services.add(&setup);
    services.add(&bootstrap);
    services.add(&system_service);
    services.add(&notification_service);
    services.add(&clock_service);
    services.add(&market_service);
    services.add(&forex_service);
    services.add(&weather_service);
    services.add(&network_worker);
    ESP_LOGI(TAG, "boot trace: services registered");

    QueueHandle_t action_queue = xQueueCreate(4, sizeof(std::function<void()>*));
    ESP_LOGI(TAG, "boot trace: action queue created");
    auto enqueue_action = [action_queue](std::function<void()> fn) {
        auto* copy = new std::function<void()>(std::move(fn));
        if (xQueueSend(action_queue, &copy, 0) != pdTRUE) {
            delete copy;
        }
    };

    np_bind_boot_skip([&]() {
        state.request_boot_skip();
    });
    np_bind_setup_submit([&](const OnboardingSubmission& submission) {
        enqueue_action([&state, submission]() {
            state.submit_onboarding(submission);
        });
    });
    np_bind_setup_scan([&]() {
        enqueue_action([&state]() {
            state.request_wifi_scan();
        });
    });
    np_bind_setup_step([&](OnboardingStep step) {
        state.set_onboarding_step(step);
    });
    np_bind_notifications_open([&]() {
        enqueue_action([&state]() {
            state.mark_all_notifications_read();
        });
    });
    // Navegar pela UI passa pelo StateStore (publica ScreenChanged) para o
    // app repintar a tela de destino com o estado atual imediatamente, sem
    // esperar o proximo evento periodico do dominio. Nao dispara fetch novo.
    np_bind_navigate([&](ScreenId screen) {
        state.set_screen(screen);
    });
    np_bind_ohlc_period([&](OhlcPeriod period) {
        enqueue_action([&market_service, period]() {
            market_service.request_ohlc_period(period);
        });
    });
    // Preferências: cada callback envia só o campo que mudou.
    // app_main lê as prefs atuais do StateStore, altera o campo e salva.
    np_bind_settings_time_format([&](bool fmt24h) {
        enqueue_action([&state, fmt24h]() {
            UserPreferences prefs = state.preferences();
            prefs.time_format_24h = fmt24h;
            state.set_preferences(prefs);
        });
    });
    np_bind_settings_theme([&](ThemeMode theme) {
        enqueue_action([&state, theme]() {
            UserPreferences prefs = state.preferences();
            prefs.theme = theme;
            state.set_preferences(prefs);
        });
    });
    np_bind_settings_reboot([&]() {
        enqueue_action([]() {
            esp_restart();
        });
    });
    // Preview: só PWM, chamado a cada pixel de drag — sem NVS write.
    // Sem enqueue, sem state change → zero flash na tela durante o arrasto.
    np_bind_settings_brightness_preview([](int pct) {
        bsp_display_brightness_set(pct);
    });
    // Save: chamado só no RELEASED — aplica PWM final e persiste.
    np_bind_settings_brightness([&](int pct) {
        bsp_display_brightness_set(pct);
        enqueue_action([&state, pct]() {
            UserPreferences prefs = state.preferences();
            prefs.brightness = pct;
            state.set_preferences(prefs);
        });
    });
    // Notificação de confirmação ao salvar qualquer setting.
    np_bind_settings_notify([&](const char* msg) {
        std::string text = msg;  // copia antes do enqueue (ptr aponta para stack do callback)
        enqueue_action([&state, text]() {
            NotificationItem item;
            item.title    = "Configurações";
            item.body     = text;
            item.level    = NotificationLevel::Info;
            item.category = NotificationCategory::System;
            state.push_notification(item);
        });
    });
    // Volume: ES8311 compartilha o barramento I2C com o touch (GT911).
    // O LVGL port faz polling de touch continuamente → acesso concorrente
    // sem mutex corrompe transações I2C → crash. Slider permanece visual.
    // TODO: implementar com I2C mutex compartilhado entre LVGL port e main task.

    // Flag compartilhado entre o render callback e o loop principal (mesma
    // thread). Evita múltiplas chamadas a np_update_home() dentro da mesma
    // janela de process_pending() quando vários providers publicam em burst
    // (Market, Weather, Clock, System na mesma frame de 200ms → flicker).
    bool home_needs_update = false;
    bool market_needs_update = false;
    bool shell_needs_update = false;
    bool settings_needs_update = false;
    bool weather_needs_update = false;

    ui.bind_render([&](const UiEvent& event) {
        if (event.source == EventType::ScreenChanged) {
            const auto screen = static_cast<ScreenId>(event.i32);
            np_navigate_to(screen);
            if (screen == ScreenId::Home) {
                home_needs_update = true;  // atualiza após navegar, fora do loop de eventos
            } else if (screen == ScreenId::Market) {
                market_needs_update = true;
            } else if (screen == ScreenId::Settings) {
                settings_needs_update = true;
            } else if (screen == ScreenId::Weather) {
                weather_needs_update = true;
            }
        } else if (event.source == EventType::BootStateChanged) {
            np_update_boot(state.state().boot);
        } else if (event.source == EventType::ClockUpdated       ||
                   event.source == EventType::MarketUpdated      ||
                   event.source == EventType::ForexUpdated       ||
                   event.source == EventType::WeatherUpdated) {
            home_needs_update = true;  // coalesced: chamada única após drenar a fila
            if (event.source == EventType::MarketUpdated ||
                event.source == EventType::ForexUpdated) {
                market_needs_update = true;
            } else if (event.source == EventType::WeatherUpdated) {
                weather_needs_update = true;
            }
        } else if (event.source == EventType::OhlcUpdated) {
            market_needs_update = true;  // so chart refreshes; no home update needed
        } else if (event.source == EventType::NotificationCreated) {
            shell_needs_update = true;
            home_needs_update = true;
            settings_needs_update = true;
            weather_needs_update = true;
            // Toast apenas para notificações novas (i32 = ID atribuído > 0).
            // i32 == 0 é o evento de mark_all_read — só atualiza o badge, sem toast.
            if (event.i32 > 0) {
                const AppState snap = state.state();
                if (!snap.notifications.items.empty()) {
                    const NotificationItem& n = snap.notifications.items.back();
                    np_show_toast(n.title.c_str(), n.body.c_str(), n.level);
                }
            }
        } else if (event.source == EventType::SystemStatusChanged) {
            shell_needs_update = true;
            home_needs_update = true;
            settings_needs_update = true;
            weather_needs_update = true;
        } else if (event.source == EventType::OnboardingStateChanged) {
            shell_needs_update = true;
            settings_needs_update = true;
            weather_needs_update = true;
            np_update_setup(state.state());
        }
    });
    ESP_LOGI(TAG, "boot trace: UI render bound");

    ESP_LOGI(TAG, "booting rewritten NovaPanel firmware");
    log_heap_snapshot("boot");

    const bool network_transport_started = start_network_transport();
    const bool cache_ready = cache.mount();

    if (!services.init_all()) {
        ESP_LOGW(TAG, "one or more services failed to init");
    }
    services.start_all();

    start_display();
    int lvgl_lock_failures = 0;
    with_lvgl_lock("initializing UI", lvgl_lock_failures, [&]() {
        const AppState current = state.state();
        np_init();
        np_update_shell(current);
        np_update_boot(current.boot);
        np_update_setup(current);
        np_update_home(current);
    });

    {
        SystemStatus system = state.state().system;
        system.board_ready = true;
        system.display_ready = true;
        system.network_ready = network_transport_started;
        system.cache_ready = cache_ready;
        state.set_system_status(system);
    }

    const uint32_t boot_now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    services.tick_all(boot_now_ms);

    vTaskDelay(pdMS_TO_TICKS(250));
    bsp_display_backlight_on();
    // Restaura brilho salvo nas preferências (default 78 se nunca alterado).
    bsp_display_brightness_set(state.preferences().brightness);
    ESP_LOGI(TAG, "backlight enabled after first UI frame");
    with_lvgl_lock("rendering initial screen", lvgl_lock_failures, [&]() {
        ui.process_pending();
    });
    log_heap_snapshot("display_ready");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(200));
        const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        requests.update_clock(now_ms);

        std::function<void()>* pending_action = nullptr;
        while (xQueueReceive(action_queue, &pending_action, 0) == pdTRUE) {
            (*pending_action)();
            delete pending_action;
        }
        services.tick_all(now_ms);
        with_lvgl_lock("periodic UI tick", lvgl_lock_failures, [&]() {
            np_tick();
            home_needs_update    = false;
            market_needs_update  = false;
            shell_needs_update   = false;
            settings_needs_update = false;
            weather_needs_update = false;
            ui.process_pending();
            const AppState current = state.state();
            if (shell_needs_update) {
                np_update_shell(current);
                shell_needs_update = false;
            }
            if (home_needs_update) {
                np_update_home(current);
                home_needs_update = false;
            }
            if (market_needs_update) {
                np_update_market(current, state.btc_ohlc());
                market_needs_update = false;
            }
            if (settings_needs_update) {
                np_update_settings(current);
                settings_needs_update = false;
            }
            if (weather_needs_update) {
                np_update_weather(current);
                weather_needs_update = false;
            }
        });
    }
}
