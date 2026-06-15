// NovaPainel - core/state_store.hpp
// Single source of truth for runtime application state.
//
// Services mutate state ONLY through the StateStore. Each mutation publishes the
// matching EventType on the EventBus so the UI (via UiDispatcher) can refresh.
// The UI reads from here; it never reads services directly. See ADR-0007.
#pragma once

#include "app_state.hpp"
#include "event_bus.hpp"

namespace nova {

class StateStore {
public:
    explicit StateStore(EventBus& bus) : bus_(bus) {}

    const AppState& state() const { return state_; }

    void set_screen(ScreenId screen);
    void set_clock(const ClockState& clock);
    void set_market(const MarketSummary& market);
    void set_system_status(const SystemStatus& status);

private:
    EventBus& bus_;
    AppState  state_{};
};

}  // namespace nova
