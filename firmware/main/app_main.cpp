// NovaPainel - main/app_main.cpp
// Firmware entry point. Wires the skeleton together with MOCKS only:
//   - no LVGL, no real display/touch driver, no real network/APIs.
// It demonstrates the data flow that the real product keeps:
//   Provider -> Service -> StateStore -> EventBus -> UiDispatcher -> (lvgl_task)
//
// Boot order (per docs/ARCHITECTURE.md):
//   EventBus -> StateStore -> UiDispatcher -> RequestOrchestrator -> Board
//   -> providers -> services -> register -> board bring-up -> init -> start
//   -> HomeScreen (logs).
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "event_bus.hpp"
#include "state_store.hpp"
#include "service_manager.hpp"
#include "request_orchestrator.hpp"
#include "ui_dispatcher.hpp"

#include "mock_board.hpp"
#include "mock_market_provider.hpp"

#include "clock_service.hpp"
#include "market_service.hpp"
#include "notification_service.hpp"

#include "home_screen.hpp"

namespace {
constexpr const char* kTag = "app_main";

uint32_t now_ms() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}
}  // namespace

extern "C" void app_main(void) {
    using namespace nova;

    ESP_LOGI(kTag, "==== NovaPainel firmware skeleton (mocks only) ====");

    // ---- Core wiring ----
    EventBus            bus;
    StateStore          store(bus);
    UiDispatcher        ui_dispatcher(bus);
    RequestOrchestrator orchestrator;

    // ---- Hardware abstraction (mock) ----
    MockBoard board;

    // ---- Providers (mock) ----
    MockMarketProvider market_provider;

    // ---- Services ----
    ClockService        clock_service(store);
    MarketService       market_service(store, orchestrator, market_provider);
    NotificationService notification_service(bus);

    ServiceManager services;
    services.add(&clock_service);
    services.add(&market_service);
    services.add(&notification_service);

    // ---- UI (log-only). Bound through the dispatcher so rendering is always
    //      marshaled - never called directly from a service. ----
    HomeScreen home;
    ui_dispatcher.bind_render([&home, &store](const UiEvent&) {
        home.render(store.state());
    });

    // ---- Board bring-up -> publish system status ----
    const BoardStatus bs = board.bring_up();
    SystemStatus sys{};
    sys.board_ready   = bs.board_ready;
    sys.display_ready = bs.display_ready;
    sys.touch_ready   = bs.touch_ready;
    sys.network_ready = bs.network_ready;
    sys.cache_ready   = false;  // no SD/cache layer yet
    store.set_system_status(sys);

    // ---- Init + start services ----
    if (!services.init_all()) {
        ESP_LOGE(kTag, "one or more services failed to init");
    }
    services.start_all();

    notification_service.notify(NotificationLevel::Info,
                                NotificationCategory::System,
                                "Boot", "NovaPainel skeleton iniciado");

    store.set_screen(ScreenId::Home);
    bus.publish(EventType::BootCompleted);

    ESP_LOGI(kTag, "entering main loop (mock). Real lvgl_task replaces this later.");

    // ---- Cooperative main loop ----
    // NOTE: in the real firmware, rendering happens on a dedicated lvgl_task and
    // services run on their own tasks. Here a single loop ticks services and
    // drains the UiDispatcher to keep the skeleton dependency-free.
    const TickType_t loop_delay = pdMS_TO_TICKS(1000);
    uint32_t last_render_ms = 0;
    for (;;) {
        const uint32_t t = now_ms();
        orchestrator.update_clock(t);
        services.tick_all(t);

        // Throttle the mock "frame" to ~once every 5s so logs stay readable.
        if (t - last_render_ms >= 5000u) {
            ui_dispatcher.process_pending();
            last_render_ms = t;
        }
        vTaskDelay(loop_delay);
    }
}
