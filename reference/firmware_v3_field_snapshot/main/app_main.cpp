// NovaPainel - main/app_main.cpp
// Firmware entry point. Board bring-up is REAL (Fase 3, WaveshareBoard),
// Boot/Setup(wizard)/Home render REAL LVGL widgets (Fase 4/5, ADR-0017),
// SetupService persists to NVS + connects Wi-Fi + starts NTP (ADR-0021).
// MarketService pulls real BTC/USD-BRL from CoinGeckoProvider (ADR-0006) and
// WeatherService pulls real weather from OpenMeteoProvider (ADR-0022), both
// gated by RequestOrchestrator. Both tick() synchronously on this loop's own
// task (not the lvgl_task) - a slow HTTPS response just delays this loop's
// next iteration for a second or two, it does not freeze rendering/touch the
// way the wizard's old direct-call bug did.
// It demonstrates the data flow that the real product keeps:
//   Provider -> Service -> StateStore -> EventBus -> UiDispatcher -> lvgl_task
//
// Boot order (per docs/ARCHITECTURE.md):
//   EventBus -> StateStore -> UiDispatcher -> RequestOrchestrator -> Board
//   -> providers -> services (incl. SetupService::init(), which decides
//   onboarding.needed from NVS) -> register -> board bring-up -> init ->
//   start -> BootScreen (LVGL) -> (after kBootScreenDurationMs) -> either
//   WizardScreen (first boot) or HomeScreen.
//
// LVGL is not thread-safe (ADR-0007/0013). The real board's own LVGL port
// task is the only task that calls lv_timer_handler(); any other code that
// touches lv_obj_* (here, the render callback driven by UiDispatcher) must
// hold board.lock()/unlock() first - see IBoard::lock in mock_board.hpp.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <functional>

#include "esp_log.h"
#include "esp_timer.h"

#include "event_bus.hpp"
#include "state_store.hpp"
#include "service_manager.hpp"
#include "request_orchestrator.hpp"
#include "ui_dispatcher.hpp"

#include "waveshare_board.hpp"
#include "cache_store.hpp"
#include "coingecko_provider.hpp"
#include "open_meteo_provider.hpp"
#include "forex_provider.hpp"

#include "clock_service.hpp"
#include "market_service.hpp"
#include "weather_service.hpp"
#include "forex_service.hpp"
#include "system_service.hpp"
#include "notification_service.hpp"
#include "setup_service.hpp"

#include "home_screen.hpp"
#include "boot_screen.hpp"
#include "wizard_screen.hpp"
#include "main_shell.hpp"
#include "system_screen.hpp"
#include "settings_screen.hpp"

#include "esp_system.h"

namespace {
constexpr const char* kTag = "app_main";

// How long the Boot/splash screen (ADR-0017/0023) stays up before the app
// moves on (unless skipped) - also drives BootScreen's progress bar
// animation. 3s (not the original 1.5s) so the once-per-second main loop
// below gets a few renders in to animate it, not just a single jump.
constexpr uint32_t kBootScreenDurationMs = 3000;

uint32_t now_ms() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}
}  // namespace

extern "C" void app_main(void) {
    using namespace nova;

    ESP_LOGI(kTag, "==== NovaPainel firmware (Fase 3: real board bring-up) ====");

    // ---- Core wiring ----
    EventBus            bus;
    StateStore          store(bus);
    UiDispatcher        ui_dispatcher(bus);
    RequestOrchestrator orchestrator;

    // ---- Hardware abstraction (real, Fase 3) ----
    WaveshareBoard board;

    // ---- Local cache (LittleFS, Fase 6/ADR-0027) ----
    CacheStore cache_store;

    // ---- Providers ----
    CoinGeckoProvider market_provider;
    OpenMeteoProvider weather_provider;
    ForexProvider     forex_provider;

    // ---- Services ----
    ClockService        clock_service(store);
    MarketService       market_service(store, orchestrator, market_provider, cache_store);
    WeatherService      weather_service(store, orchestrator, weather_provider, cache_store);
    ForexService        forex_service(store, orchestrator, forex_provider, cache_store);
    SystemService       system_service(store);
    NotificationService notification_service(bus);
    SetupService        setup_service(store, bus);

    ServiceManager services;
    services.add(&clock_service);
    services.add(&market_service);
    services.add(&weather_service);
    services.add(&forex_service);
    services.add(&system_service);
    services.add(&notification_service);
    services.add(&setup_service);

    // SetupService does NVS writes + esp_wifi_connect()/esp_wifi_scan_start()
    // - all of which can block for a while (flash I/O, Wi-Fi RPC over the
    // Hosted/SDIO link). WizardScreen's button-click callbacks run ON the
    // BSP's own lvgl_task WHILE it holds the BSP's LVGL mutex (ADR-0018) -
    // calling store.submit_onboarding()/request_wifi_scan() straight from
    // there froze the whole display for as long as the call took (found on
    // real hardware testing this wizard). This queue defers any such action
    // to this loop's own task, off the lvgl_task, so a slow/failing Wi-Fi
    // call never blocks rendering/touch. Holds heap-allocated std::function
    // pointers (not OnboardingSubmission directly) so it can carry either
    // kind of deferred call through the same queue.
    QueueHandle_t action_queue = xQueueCreate(4, sizeof(std::function<void()>*));
    auto enqueue_action = [action_queue](std::function<void()> fn) {
        auto* copy = new std::function<void()>(std::move(fn));
        if (xQueueSend(action_queue, &copy, 0) != pdTRUE) {
            delete copy;  // queue full - drop rather than block the lvgl_task
        }
    };

    // ---- UI (real LVGL widgets, Fase 4/5). Bound through the dispatcher so
    //      rendering is always marshaled - never called directly from a
    //      service - and always under the board's LVGL lock. WizardScreen's
    //      callbacks are the only way the UI reaches SetupService
    //      (ADR-0017: UI never persists/calls hardware itself). ----
    BootScreen boot(kBootScreenDurationMs, [&store]() {
        // Cheap in-memory state change (no NVS/Wi-Fi) - safe to call
        // directly off the lvgl_task, unlike the wizard's actions above.
        const AppState state = store.snapshot();
        store.set_screen(state.onboarding.needed ? ScreenId::Setup
                                                 : ScreenId::Home);
    });
    WizardScreen wizard(
        [enqueue_action, &store](const OnboardingSubmission& submission) {
            enqueue_action([&store, submission] { store.submit_onboarding(submission); });
        },
        [enqueue_action, &store]() {
            enqueue_action([&store] { store.request_wifi_scan(); });
        });
    HomeScreen   home;
    MainShell    shell([&store](ScreenId id) { store.set_screen(id); });
    SystemScreen system_screen([&store]() { store.set_screen(ScreenId::Home); });
    SettingsScreen settings_screen(
        [enqueue_action, &store](const OnboardingSubmission& sub) {
            enqueue_action([&store, sub] { store.submit_onboarding(sub); });
        },
        [&store](ScreenId id) { store.set_screen(id); },
        [enqueue_action, &store]() {
            enqueue_action([&store] { store.request_wifi_scan(); });
        },
        [enqueue_action]() {
            enqueue_action([] { esp_restart(); });
        });
    ui_dispatcher.bind_render([&boot, &wizard, &home, &shell, &system_screen,
                               &settings_screen, &store, &board](const UiEvent&) {
        if (board.lock(100)) {  // 100ms, in real ms (not ticks) - see IBoard::lock
            const AppState state = store.snapshot();
            switch (state.current_screen) {
                case ScreenId::Boot:     boot.render(state); break;
                case ScreenId::Setup:    wizard.render(state); break;
                // Own top-level screen, not mounted via MainShell::content()
                // - see system_screen.hpp for why (Fase 7).
                case ScreenId::System:   system_screen.render(state, now_ms()); break;
                case ScreenId::Settings: settings_screen.render(state); break;
                default:
                    // MainShell (ADR-0024) owns the lv_screen_load() for
                    // every "MAIN phase" screen; HomeScreen just mounts its
                    // widgets inside shell.content() on first call.
                    shell.render(state);
                    home.render(state, shell.content());
                    break;
            }
            board.unlock();
        }
    });

    // ---- Board bring-up -> publish system status ----
    const BoardStatus bs = board.bring_up();
    SystemStatus sys{};
    sys.board_ready   = bs.board_ready;
    sys.display_ready = bs.display_ready;
    sys.touch_ready   = bs.touch_ready;
    sys.network_ready = bs.network_ready;
    sys.sd_ready      = bs.sd_ready;
    sys.cache_ready   = cache_store.mount();  // LittleFS on "storage" (Fase 6, ADR-0027) - false is not fatal, services just skip cache reads/writes
    store.set_system_status(sys);

    // ---- Init + start services ----
    if (!services.init_all()) {
        ESP_LOGE(kTag, "one or more services failed to init");
    }
    services.start_all();

    notification_service.notify(NotificationLevel::Info,
                                NotificationCategory::System,
                                "Boot", "NovaPainel skeleton iniciado");

    bus.publish(EventType::BootCompleted);

    // Render the Boot/splash screen right away (don't wait for the 5s frame
    // throttle below) - SystemStatusChanged was already queued by
    // store.set_system_status() above.
    ui_dispatcher.process_pending();

    // Only expose the panel after the first app-controlled screen has been
    // built and queued. The panel-default frame becomes visible if the
    // backlight comes on before LVGL has completed its first solid paint, so
    // give the BSP task a longer window to flush BootScreen first.
    vTaskDelay(pdMS_TO_TICKS(1200));
    if (!board.enable_backlight()) {
        ESP_LOGW(kTag, "display backlight stayed off after first frame");
    }

    ESP_LOGI(kTag, "entering main loop. LVGL itself runs on the board's own lvgl_task.");

    // ---- Cooperative main loop ----
    // NOTE: the real LVGL task (inside the BSP's esp_lvgl_port) calls
    // lv_timer_handler() on its own; this loop just ticks services and drains
    // the UiDispatcher into HomeScreen::render() under the board's LVGL lock
    // (see bind_render above). Services running on their own tasks remains
    // future work; this single loop keeps the skeleton dependency-free.
    const TickType_t loop_delay = pdMS_TO_TICKS(1000);
    const uint32_t   boot_start_ms = now_ms();
    uint32_t         last_render_ms = 0;
    bool             splash_done = false;
    for (;;) {
        const uint32_t t = now_ms();
        orchestrator.update_clock(t);
        services.tick_all(t);

        bool render_now = false;

        // Drain deferred wizard actions here - off the lvgl_task - so
        // SetupService's NVS/Wi-Fi calls never block rendering or touch
        // (see action_queue comment above).
        std::function<void()>* pending_action = nullptr;
        while (xQueueReceive(action_queue, &pending_action, 0) == pdTRUE) {
            (*pending_action)();
            delete pending_action;
            render_now = true;
        }

        if (!splash_done && t - boot_start_ms >= kBootScreenDurationMs) {
            // SetupService::init() already decided onboarding.needed from NVS
            // before this loop started.
            const AppState state = store.snapshot();
            store.set_screen(state.onboarding.needed ? ScreenId::Setup
                                                     : ScreenId::Home);
            splash_done = true;
            render_now = true;
        } else {
            const AppState state = store.snapshot();
            if (state.current_screen == ScreenId::Setup &&
                state.onboarding.step == OnboardingStep::Done) {
                store.set_screen(ScreenId::Home);
                render_now = true;
            }
        }

        // 1s matches this loop's own tick and gives per-second clock updates
        // without a separate LVGL timer. Dirty-rect render is cheap: only
        // changed label regions are flushed, not the full screen.
        const uint32_t render_interval_ms = 1000u;
        if (render_now || t - last_render_ms >= render_interval_ms) {
            ui_dispatcher.process_pending();
            last_render_ms = t;
        }
        vTaskDelay(loop_delay);
    }
}
