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
    // Updates only the usd_brl/usd_brl_valid/usd_brl_source/usd_brl_last_update_ms
    // fields, read-modify-write - ForexService and MarketService fetch
    // independently and must not clobber each other's half of MarketSummary.
    void set_usd_brl_rate(double rate, DataSource source, uint32_t now_ms);
    void set_weather(const WeatherSummary& weather);
    void set_system_status(const SystemStatus& status);

    // ---- Onboarding wizard (ADR-0017) ----
    // UI side: publishes the wizard's "intention", never persists/calls
    // hardware itself. SetupService is the only subscriber that acts on it.
    void submit_onboarding(const OnboardingSubmission& submission);
    // UI side: asks for a fresh Wi-Fi scan (same "intention, never touches
    // hardware" rule). Optimistically flips wifi_scan_status to Scanning so
    // the wizard shows feedback before SetupService's real scan finishes.
    void request_wifi_scan();
    // SetupService side: writes back results. Each publishes
    // OnboardingStateChanged so the wizard/UI re-renders.
    void set_onboarding_needed(bool needed);
    void set_onboarding_step(OnboardingStep step);
    void set_wifi_status(WifiConnectStatus status);
    void set_preferences(const UserPreferences& preferences);
    void set_wifi_networks(const std::vector<WifiNetwork>& networks, WifiScanStatus status);

private:
    EventBus& bus_;
    AppState  state_{};
};

}  // namespace nova
