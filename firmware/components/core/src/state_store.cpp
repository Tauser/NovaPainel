// NovaPanel - core/state_store.cpp
#include "state_store.hpp"

namespace nova {

AppState StateStore::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

void StateStore::set_screen(ScreenId screen) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.current_screen = screen;
    }
    bus_.publish(EventType::ScreenChanged, static_cast<int32_t>(screen));
}

void StateStore::set_boot_state(const BootState& boot) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.boot = boot;
    }
    bus_.publish(EventType::BootStateChanged);
}

void StateStore::request_boot_skip() {
    bus_.publish(EventType::BootSkipRequested);
}

void StateStore::set_clock(const ClockState& clock) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.clock = clock;
    }
    bus_.publish(EventType::ClockUpdated);
}

void StateStore::set_crypto(const CryptoSummary& crypto) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.crypto = crypto;
    }
    bus_.publish(EventType::MarketUpdated);
}

void StateStore::set_forex(const ForexSummary& forex) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.forex = forex;
    }
    bus_.publish(EventType::ForexUpdated);
}

void StateStore::set_boot_diagnostics(const char* reset_reason, uint32_t reboot_count) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.system.reset_reason = reset_reason;
        state_.system.reboot_count = reboot_count;
    }
    bus_.publish(EventType::SystemStatusChanged);
}

void StateStore::set_weather(const WeatherSummary& weather) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.weather = weather;
    }
    bus_.publish(EventType::WeatherUpdated);
}

void StateStore::set_system_status(const SystemStatus& status) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.system = status;
    }
    bus_.publish(EventType::SystemStatusChanged);
}

void StateStore::submit_onboarding(const OnboardingSubmission& submission) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.onboarding.pending_submission = submission;
    }
    bus_.publish(EventType::OnboardingStepSubmitted, static_cast<int32_t>(submission.step));
}

void StateStore::request_wifi_scan() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.onboarding.wifi_scan_status = WifiScanStatus::Scanning;
    }
    bus_.publish(EventType::WifiScanRequested);
}

void StateStore::set_onboarding_needed(bool needed) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.onboarding.needed = needed;
    }
    bus_.publish(EventType::OnboardingStateChanged);
}

void StateStore::set_onboarding_step(OnboardingStep step) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.onboarding.step = step;
    }
    bus_.publish(EventType::OnboardingStateChanged, static_cast<int32_t>(step));
}

void StateStore::set_wifi_status(WifiConnectStatus status) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.onboarding.wifi_status = status;
    }
    bus_.publish(EventType::OnboardingStateChanged);
}

void StateStore::set_preferences(const UserPreferences& preferences) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.preferences = preferences;
    }
    bus_.publish(EventType::OnboardingStateChanged);
}

void StateStore::set_wifi_networks(const std::vector<WifiNetwork>& networks,
                                   WifiScanStatus status) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.onboarding.wifi_networks = networks;
        state_.onboarding.wifi_scan_status = status;
    }
    bus_.publish(EventType::OnboardingStateChanged);
}

}  // namespace nova
