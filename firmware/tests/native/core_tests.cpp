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

}  // namespace

int main()
{
    test_event_bus_publish();
    test_state_store_split_updates();
    test_request_orchestrator_circuit();
    return 0;
}
