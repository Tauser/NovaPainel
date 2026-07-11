#pragma once

#include "app_state.hpp"
#include "event_bus.hpp"

#include <mutex>

namespace nova {

class StateStore {
public:
    explicit StateStore(EventBus& event_bus);

    AppState snapshot() const;
    ClockState clock() const;
    MarketState market() const;
    WeatherState weather() const;
    SystemState system() const;
    UiState ui() const;

    void set_clock(ClockState clock);
    void set_market(MarketState market);
    void set_weather(WeatherState weather);
    void set_system(SystemState system);
    void set_ui(UiState ui);
    void navigate_to(ScreenId screen_id);
    void set_action_queue_overflows(uint32_t overflow_count);

private:
    EventBus& event_bus_;
    // Shared by main_loop and future net_worker/service tasks. All access goes
    // through state_mutex_ so EventBus remains signal-only and data stays here.
    mutable std::mutex state_mutex_{};
    AppState state_{};
};

}  // namespace nova
