// NovaPainel - core/state_store.cpp
#include "state_store.hpp"

namespace nova {

void StateStore::set_screen(ScreenId screen) {
    state_.current_screen = screen;
    bus_.publish(EventType::ScreenChanged, static_cast<int32_t>(screen));
}

void StateStore::set_clock(const ClockState& clock) {
    state_.clock = clock;
    bus_.publish(EventType::ClockUpdated);
}

void StateStore::set_market(const MarketSummary& market) {
    state_.market = market;
    bus_.publish(EventType::MarketUpdated);
}

void StateStore::set_system_status(const SystemStatus& status) {
    state_.system = status;
    bus_.publish(EventType::SystemStatusChanged);
}

}  // namespace nova
