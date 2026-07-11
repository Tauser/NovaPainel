#include <cassert>
#include <vector>

#include "event_bus.hpp"
#include "request_orchestrator.hpp"
#include "state_store.hpp"

using namespace nova;

namespace {

void test_event_bus_publish()
{
    EventBus bus;
    std::vector<EventType> seen;
    bus.subscribe([&](const Event& event) {
        seen.push_back(event.type);
    });

    bus.publish(EventType::ClockUpdated);
    bus.publish(EventType::ForexUpdated);

    assert(seen.size() == 2);
    assert(seen[0] == EventType::ClockUpdated);
    assert(seen[1] == EventType::ForexUpdated);
}

void test_state_store_split_updates()
{
    EventBus bus;
    StateStore store(bus);
    std::vector<EventType> seen;
    bus.subscribe([&](const Event& event) {
        seen.push_back(event.type);
    });

    CryptoSummary crypto{};
    crypto.btc_usd = 123456.0;
    crypto.valid = true;
    crypto.source = DataSource::Live;
    store.set_crypto(crypto);

    ForexSummary forex{};
    forex.usd_brl = 5.42;
    forex.usd_brl_valid = true;
    forex.usd_brl_source = DataSource::Cache;
    store.set_forex(forex);

    const AppState snapshot = store.snapshot();
    assert(snapshot.crypto.btc_usd == 123456.0);
    assert(snapshot.crypto.valid);
    assert(snapshot.forex.usd_brl == 5.42);
    assert(snapshot.forex.usd_brl_valid);
    assert(seen.size() == 2);
    assert(seen[0] == EventType::MarketUpdated);
    assert(seen[1] == EventType::ForexUpdated);
}

void test_request_orchestrator_circuit()
{
    RequestOrchestrator orchestrator;
    orchestrator.update_clock(0);
    assert(orchestrator.can_request(DataDomain::Forex));

    orchestrator.note_request(DataDomain::Forex, 1000, false);
    orchestrator.note_request(DataDomain::Forex, 2000, false);
    orchestrator.note_request(DataDomain::Forex, 3000, false);
    assert(orchestrator.circuit_state(DataDomain::Forex) == CircuitState::Open);
    assert(!orchestrator.can_request(DataDomain::Forex));

    orchestrator.update_clock(20000);
    assert(orchestrator.circuit_state(DataDomain::Forex) == CircuitState::HalfOpen);
}

void test_state_store_notifications()
{
    EventBus bus;
    StateStore store(bus);
    std::vector<EventType> seen;
    bus.subscribe([&](const Event& event) {
        seen.push_back(event.type);
    });

    NotificationItem info{};
    info.level = NotificationLevel::Info;
    info.title = "Info";
    info.timestamp_ms = 10;
    store.push_notification(info);

    NotificationItem danger{};
    danger.level = NotificationLevel::Danger;
    danger.title = "Danger";
    danger.timestamp_ms = 20;
    store.push_notification(danger);

    const AppState snapshot = store.snapshot();
    assert(snapshot.notifications.items.size() == 2);
    assert(snapshot.notifications.items[0].title == "Danger");
    assert(snapshot.notifications.items[1].title == "Info");
    assert(snapshot.notifications.unread_count == 2);
    assert(seen.size() == 2);
    assert(seen[0] == EventType::NotificationCreated);
    assert(seen[1] == EventType::NotificationCreated);

    store.mark_all_notifications_read();
    const AppState updated = store.snapshot();
    assert(updated.notifications.unread_count == 0);
    assert(updated.notifications.items[0].read);
    assert(updated.notifications.items[1].read);
}

}  // namespace

int main()
{
    test_event_bus_publish();
    test_state_store_split_updates();
    test_request_orchestrator_circuit();
    test_state_store_notifications();
    return 0;
}
