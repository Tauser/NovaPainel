#include <cstdio>
#include <cstdlib>

#include "event_bus.hpp"
#include "notification_service.hpp"
#include "request_orchestrator.hpp"
#include "state_store.hpp"
#include "ui_dispatcher.hpp"

namespace {

void fail(const char* message) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
}

void expect(bool condition, const char* message) {
    if (!condition) fail(message);
}

void test_state_store_market_merge() {
    nova::EventBus bus;
    nova::StateStore store(bus);

    store.set_usd_brl_rate(5.42, nova::DataSource::Live, 111u);

    nova::MarketSummary market{};
    market.btc_usd = 68000.0;
    market.btc_change_24h = 4.5;
    market.valid = true;
    market.source = nova::DataSource::Live;
    market.last_update_ms = 222u;
    store.set_market(market);

    const nova::AppState state = store.snapshot();
    expect(state.market.btc_usd == 68000.0, "set_market should update btc_usd");
    expect(state.market.usd_brl == 5.42, "set_market should preserve usd_brl");
    expect(state.market.usd_brl_valid, "set_market should preserve usd_brl_valid");
    expect(state.market.usd_brl_last_update_ms == 111u,
           "set_market should preserve usd_brl timestamp");
}

void test_request_orchestrator_interval_and_circuit() {
    nova::RequestOrchestrator orchestrator;

    orchestrator.update_clock(1000u);
    expect(orchestrator.can_request(nova::DataDomain::MarketSummary),
           "market summary should allow the first request");
    orchestrator.note_request(nova::DataDomain::MarketSummary, 1000u, true);

    orchestrator.update_clock(2000u);
    expect(!orchestrator.can_request(nova::DataDomain::MarketSummary),
           "market summary should respect the 60s interval");

    orchestrator.note_request(nova::DataDomain::Weather, 1000u, false);
    orchestrator.note_request(nova::DataDomain::Weather, 2000u, false);
    orchestrator.note_request(nova::DataDomain::Weather, 3000u, false);
    expect(orchestrator.circuit_state(nova::DataDomain::Weather) == nova::CircuitState::Open,
           "weather should open the circuit after three consecutive failures");
    expect(orchestrator.is_degraded(nova::DataDomain::Weather),
           "open circuit should report degraded state");

    orchestrator.update_clock(1000000u);
    expect(orchestrator.circuit_state(nova::DataDomain::Weather) == nova::CircuitState::HalfOpen,
           "weather circuit should probe after backoff expires");
}

void test_event_bus_unsubscribe_during_publish() {
    nova::EventBus bus;

    int first_calls = 0;
    int second_calls = 0;
    nova::SubscriptionId first_id = 0;

    first_id = bus.subscribe([&](const nova::Event&) {
        ++first_calls;
    });
    bus.subscribe([&](const nova::Event&) {
        ++second_calls;
        bus.unsubscribe(first_id);
    });

    bus.publish(nova::EventType::ClockUpdated);
    bus.publish(nova::EventType::ClockUpdated);

    expect(first_calls == 1, "unsubscribe during publish should affect later publishes only");
    expect(second_calls == 2, "remaining subscriber should keep receiving events");
}

void test_ui_dispatcher_filters_and_coalesces() {
    nova::EventBus bus;
    nova::UiDispatcher dispatcher(bus);

    int render_calls = 0;
    nova::EventType last_source = nova::EventType::BootCompleted;
    dispatcher.bind_render([&](const nova::UiEvent& event) {
        ++render_calls;
        last_source = event.source;
    });

    bus.publish(nova::EventType::ClockUpdated);
    bus.publish(nova::EventType::RequestPolicyChanged);
    bus.publish(nova::EventType::MarketUpdated);

    expect(dispatcher.pending() == 2u, "dispatcher should queue only UI-relevant events");
    dispatcher.process_pending();

    expect(render_calls == 1, "dispatcher should coalesce pending events into one render");
    expect(last_source == nova::EventType::MarketUpdated,
           "dispatcher should render using the most recent UI event");
    expect(dispatcher.pending() == 0u, "dispatcher should drain the queue");
}

void test_notification_service_eviction() {
    nova::EventBus bus;
    nova::NotificationService service(bus);

    for (int i = 0; i < 32; ++i) {
        service.notify(nova::NotificationLevel::Silent,
                       nova::NotificationCategory::System,
                       "silent", "", static_cast<uint32_t>(i));
    }
    service.notify(nova::NotificationLevel::Danger,
                   nova::NotificationCategory::System,
                   "danger", "", 100u);

    expect(service.recent().size() == 32u, "notification queue should stay capped at 32");
    expect(service.unread_count() == 32, "notification queue should keep 32 unread items");

    bool found_danger = false;
    for (const auto& item : service.recent()) {
        if (item.level == nova::NotificationLevel::Danger && item.title == "danger") {
            found_danger = true;
            break;
        }
    }
    expect(found_danger, "highest-priority notification should survive queue pressure");
}

}  // namespace

int main() {
    test_state_store_market_merge();
    test_request_orchestrator_interval_and_circuit();
    test_event_bus_unsubscribe_during_publish();
    test_ui_dispatcher_filters_and_coalesces();
    test_notification_service_eviction();
    std::puts("core_tests: PASS");
    return 0;
}
