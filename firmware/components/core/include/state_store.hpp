#pragma once

#include "app_state.hpp"
#include "event_bus.hpp"

namespace nova {

class StateStore {
public:
    explicit StateStore(EventBus& event_bus);

    AppState snapshot() const { return state_; }
    ClockState clock() const { return state_.clock; }
    MarketState market() const { return state_.market; }
    WeatherState weather() const { return state_.weather; }
    SystemState system() const { return state_.system; }

    void set_clock(ClockState clock);
    void set_market(MarketState market);
    void set_weather(WeatherState weather);
    void set_system(SystemState system);
    void set_action_queue_overflows(uint32_t overflow_count);

private:
    EventBus& event_bus_;
    AppState state_{};
};

}  // namespace nova
