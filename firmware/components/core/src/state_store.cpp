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
    // MarketService only ever fills the btc_*/valid/stale/source/
    // last_update_ms half of MarketSummary (a fresh `MarketSummary{}` has
    // usd_brl_valid=false) - preserve whatever ForexService already set
    // independently instead of letting this full-struct assignment wipe it.
    const double     usd_brl = state_.market.usd_brl;
    const bool       usd_brl_valid = state_.market.usd_brl_valid;
    const bool       usd_brl_stale = state_.market.usd_brl_stale;
    const DataSource usd_brl_source = state_.market.usd_brl_source;
    const uint32_t   usd_brl_last_update_ms = state_.market.usd_brl_last_update_ms;

    state_.market = market;

    state_.market.usd_brl = usd_brl;
    state_.market.usd_brl_valid = usd_brl_valid;
    state_.market.usd_brl_stale = usd_brl_stale;
    state_.market.usd_brl_source = usd_brl_source;
    state_.market.usd_brl_last_update_ms = usd_brl_last_update_ms;

    bus_.publish(EventType::MarketUpdated);
}

void StateStore::set_usd_brl_rate(double rate, DataSource source, uint32_t now_ms) {
    state_.market.usd_brl = rate;
    state_.market.usd_brl_valid = true;
    state_.market.usd_brl_stale = (source == DataSource::Cache);
    state_.market.usd_brl_source = source;
    state_.market.usd_brl_last_update_ms = now_ms;
    bus_.publish(EventType::MarketUpdated);
}

void StateStore::set_boot_diagnostics(const char* reset_reason, uint32_t reboot_count) {
    state_.system.reset_reason = reset_reason;
    state_.system.reboot_count = reboot_count;
    bus_.publish(EventType::SystemStatusChanged);
}

void StateStore::set_weather(const WeatherSummary& weather) {
    state_.weather = weather;
    bus_.publish(EventType::WeatherUpdated);
}

void StateStore::set_system_status(const SystemStatus& status) {
    state_.system = status;
    bus_.publish(EventType::SystemStatusChanged);
}

void StateStore::submit_onboarding(const OnboardingSubmission& submission) {
    state_.onboarding.pending_submission = submission;
    bus_.publish(EventType::OnboardingStepSubmitted,
                 static_cast<int32_t>(submission.step));
}

void StateStore::set_onboarding_needed(bool needed) {
    state_.onboarding.needed = needed;
    bus_.publish(EventType::OnboardingStateChanged);
}

void StateStore::set_onboarding_step(OnboardingStep step) {
    state_.onboarding.step = step;
    bus_.publish(EventType::OnboardingStateChanged, static_cast<int32_t>(step));
}

void StateStore::set_wifi_status(WifiConnectStatus status) {
    state_.onboarding.wifi_status = status;
    bus_.publish(EventType::OnboardingStateChanged);
}

void StateStore::set_preferences(const UserPreferences& preferences) {
    state_.preferences = preferences;
    bus_.publish(EventType::OnboardingStateChanged);
}

void StateStore::request_wifi_scan() {
    state_.onboarding.wifi_scan_status = WifiScanStatus::Scanning;
    bus_.publish(EventType::WifiScanRequested);
}

void StateStore::set_wifi_networks(const std::vector<WifiNetwork>& networks,
                                   WifiScanStatus status) {
    state_.onboarding.wifi_networks = networks;
    state_.onboarding.wifi_scan_status = status;
    bus_.publish(EventType::OnboardingStateChanged);
}

}  // namespace nova
