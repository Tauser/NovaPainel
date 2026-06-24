// NovaPainel - main/app_main.cpp
// Firmware entry point. Board bring-up is REAL (Fase 3, WaveshareBoard) and
// the Home screen renders REAL LVGL widgets (Fase 4). Wi-Fi AP association
// and real market data are still MOCK (Fase 5).
// It demonstrates the data flow that the real product keeps:
//   Provider -> Service -> StateStore -> EventBus -> UiDispatcher -> lvgl_task
//
// Boot order (per docs/ARCHITECTURE.md):
//   EventBus -> StateStore -> UiDispatcher -> RequestOrchestrator -> Board
//   -> providers -> services -> register -> board bring-up -> init -> start
//   -> HomeScreen (LVGL).
//
// LVGL is not thread-safe (ADR-0007/0013). The real board's own LVGL port
// task is the only task that calls lv_timer_handler(); any other code that
// touches lv_obj_* (here, the render callback driven by UiDispatcher) must
// hold board.lock()/unlock() first - see IBoard::lock in mock_board.hpp.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "event_bus.hpp"
#include "state_store.hpp"
#include "service_manager.hpp"
#include "request_orchestrator.hpp"
#include "ui_dispatcher.hpp"

#include "waveshare_board.hpp"
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

    ESP_LOGI(kTag, "==== NovaPainel firmware (Fase 3: real board bring-up) ====");

    // ---- Core wiring ----
    EventBus            bus;
    StateStore          store(bus);
    UiDispatcher        ui_dispatcher(bus);
    RequestOrchestrator orchestrator;

    // ---- Hardware abstraction (real, Fase 3) ----
    WaveshareBoard board;

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

    // ---- UI (real LVGL widgets, Fase 4). Bound through the dispatcher so
    //      rendering is always marshaled - never called directly from a
    //      service - and always under the board's LVGL lock. ----
    HomeScreen home;
    ui_dispatcher.bind_render([&home, &store, &board](const UiEvent&) {
        if (board.lock(100)) {  // 100ms, in real ms (not ticks) - see IBoard::lock
            home.render(store.state());
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
    sys.cache_ready   = false;  // LittleFS cache layer not built yet (Fase 6)
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

    ESP_LOGI(kTag, "entering main loop. LVGL itself runs on the board's own lvgl_task.");

    // ---- Cooperative main loop ----
    // NOTE: the real LVGL task (inside the BSP's esp_lvgl_port) calls
    // lv_timer_handler() on its own; this loop just ticks services and drains
    // the UiDispatcher into HomeScreen::render() under the board's LVGL lock
    // (see bind_render above). Services running on their own tasks remains
    // future work; this single loop keeps the skeleton dependency-free.
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
