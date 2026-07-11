// NovaPanel - core/state_store.hpp
#pragma once

#include <mutex>

#include "app_state.hpp"
#include "event_bus.hpp"

namespace nova {

class StateStore {
public:
    explicit StateStore(EventBus& bus) : bus_(bus) {}

    AppState state() const { return snapshot(); }
    AppState snapshot() const;
    ScreenId current_screen() const;
    ClockState clock() const;
    WeatherSummary weather() const;
    SystemStatus system_status() const;
    UserPreferences preferences() const;
    bool onboarding_needed() const;
    OnboardingStep onboarding_step() const;
    WifiConnectStatus wifi_status() const;
    OnboardingSubmission pending_onboarding_submission() const;

    void set_screen(ScreenId screen);
    void set_boot_state(const BootState& boot);
    void request_boot_skip();
    void set_clock(const ClockState& clock);
    void set_crypto(const CryptoSummary& crypto);
    void set_btc_ohlc(const OhlcSeries& ohlc);
    OhlcSeries btc_ohlc() const;   // separate accessor - not part of snapshot()
    void set_forex(const ForexSummary& forex);
    void set_weather(const WeatherSummary& weather);
    uint32_t push_notification(const NotificationItem& notification);
    void mark_all_notifications_read();
    void set_system_status(const SystemStatus& status);
    void set_boot_diagnostics(const char* reset_reason, uint32_t reboot_count);
    void submit_onboarding(const OnboardingSubmission& submission);
    void request_wifi_scan();
    void set_onboarding_needed(bool needed);
    void set_onboarding_step(OnboardingStep step);
    void set_wifi_status(WifiConnectStatus status);
    void set_preferences(const UserPreferences& preferences);
    void set_wifi_networks(const std::vector<WifiNetwork>& networks, WifiScanStatus status);

private:
    EventBus& bus_;
    mutable std::recursive_mutex mutex_;
    AppState                     state_{};
    OhlcSeries                   ohlc_{};   // kept separate so snapshot() stays small
};

}  // namespace nova
