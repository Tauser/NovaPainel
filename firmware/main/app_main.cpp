#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <functional>

#include "bsp/esp32_p4_wifi6_touch_lcd_7b.h"
#include "event_bus.hpp"
#include "clock_service.hpp"
#include "bootstrap_service.hpp"
#include "request_orchestrator.hpp"
#include "cache_store.hpp"
#include "market_service.hpp"
#include "forex_service.hpp"
#include "weather_service.hpp"
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

#define NOVAPANEL_DISPLAY_ROTATION LV_DISPLAY_ROTATION_180
#define NOVAPANEL_LVGL_LOCK_MS 1000

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
        .buffer_size = BSP_LCD_H_RES * (BSP_LCD_V_RES / 4),
        .double_buffer = false,
        .flags = {
            .buff_dma = true,
            .buff_spiram = true,
            .sw_rotate = true,
        },
    };
    cfg.lvgl_port_cfg.task_stack = 16384;

    lv_display_t *display = bsp_display_start_with_config(&cfg);
    if (display == NULL) {
        ESP_LOGE(TAG, "display start failed");
        abort();
    }

    bsp_display_rotate(display, NOVAPANEL_DISPLAY_ROTATION);
    ESP_LOGI(TAG, "display rotation=%d sw_rotate=1 double_buffer=0", (int)NOVAPANEL_DISPLAY_ROTATION);
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
    EventBus bus;
    StateStore state(bus);
    UiDispatcher ui(bus);
    RequestOrchestrator requests;
    CacheStore cache;
    ServiceManager services;
    BootstrapService bootstrap(state, bus);
    SetupService setup(state, bus);
    SystemService system_service(state);
    ClockService clock_service(state);
    CoinGeckoProvider coingecko_provider;
    ForexProvider forex_provider;
    OpenMeteoProvider open_meteo_provider;
    MarketService market_service(state, requests, cache, coingecko_provider);
    ForexService forex_service(state, requests, cache, forex_provider);
    WeatherService weather_service(state, requests, cache, open_meteo_provider);

    services.add(&setup);
    services.add(&bootstrap);
    services.add(&system_service);
    services.add(&clock_service);
    services.add(&market_service);
    services.add(&forex_service);
    services.add(&weather_service);

    QueueHandle_t action_queue = xQueueCreate(4, sizeof(std::function<void()>*));
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

    ui.bind_render([&](const UiEvent& event) {
        ESP_LOGI(TAG, "ui event queued from %s (i32=%" PRId32 ")", to_string(event.source), event.i32);
        if (event.source == EventType::ScreenChanged) {
            const auto screen = static_cast<ScreenId>(event.i32);
            np_navigate_to(screen);
            if (screen == ScreenId::Home) {
                np_update_home(state.state());
            }
        } else if (event.source == EventType::BootStateChanged) {
            np_update_boot(state.state().boot);
        } else if (event.source == EventType::ClockUpdated) {
            np_update_home(state.state());
        } else if (event.source == EventType::MarketUpdated ||
                   event.source == EventType::WeatherUpdated ||
                   event.source == EventType::SystemStatusChanged) {
            np_update_home(state.state());
        } else if (event.source == EventType::OnboardingStateChanged) {
            np_update_setup(state.state());
        }
    });

    ESP_LOGI(TAG, "booting rewritten NovaPanel firmware");
    log_heap_snapshot("boot");

    const bool network_transport_started = start_network_transport();
    const bool cache_ready = cache.mount();

    if (!services.init_all()) {
        ESP_LOGW(TAG, "one or more services failed to init");
    }
    services.start_all();

    start_display();
    if (!bsp_display_lock(NOVAPANEL_LVGL_LOCK_MS)) {
        ESP_LOGE(TAG, "LVGL lock timeout while initializing UI");
        abort();
    }
    np_init();
    np_update_boot(state.state().boot);
    np_update_setup(state.state());
    np_update_home(state.state());
    bsp_display_unlock();

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
    ESP_LOGI(TAG, "backlight enabled after first UI frame");
    if (!bsp_display_lock(NOVAPANEL_LVGL_LOCK_MS)) {
        ESP_LOGE(TAG, "LVGL lock timeout while rendering initial screen");
        abort();
    }
    ui.process_pending();
    bsp_display_unlock();
    log_heap_snapshot("display_ready");

    uint32_t last_alive_log_ms = boot_now_ms;
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
        if (!bsp_display_lock(NOVAPANEL_LVGL_LOCK_MS)) {
            ESP_LOGE(TAG, "LVGL lock timeout in periodic UI tick");
            abort();
        }
        np_tick();
        ui.process_pending();
        bsp_display_unlock();
        if (now_ms - last_alive_log_ms >= 1000u) {
            log_heap_snapshot("alive");
            last_alive_log_ms = now_ms;
        }
    }
}
