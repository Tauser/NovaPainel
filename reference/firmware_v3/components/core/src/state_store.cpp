// NovaPanel - core/state_store.cpp
#include "state_store.hpp"

#include <algorithm>

namespace nova {

namespace {
constexpr std::size_t kMaxNotifications = 16;

int notification_rank(NotificationLevel level)
{
    switch (level) {
        case NotificationLevel::Danger:  return 4;
        case NotificationLevel::Warning: return 3;
        case NotificationLevel::Success: return 2;
        case NotificationLevel::Info:    return 1;
        case NotificationLevel::Silent:  return 0;
    }
    return 0;
}

void sort_notifications(std::vector<NotificationItem>& items)
{
    std::sort(items.begin(), items.end(), [](const NotificationItem& lhs,
                                             const NotificationItem& rhs) {
        const int lhs_rank = notification_rank(lhs.level);
        const int rhs_rank = notification_rank(rhs.level);
        if (lhs_rank != rhs_rank) {
            return lhs_rank > rhs_rank;
        }
        return lhs.timestamp_ms > rhs.timestamp_ms;
    });
}
}  // namespace

AppState StateStore::snapshot() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_;
}

ScreenId StateStore::current_screen() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_.current_screen;
}

ClockState StateStore::clock() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_.clock;
}

WeatherSummary StateStore::weather() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_.weather;
}

SystemStatus StateStore::system_status() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_.system;
}

UserPreferences StateStore::preferences() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_.preferences;
}

bool StateStore::onboarding_needed() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_.onboarding.needed;
}

OnboardingStep StateStore::onboarding_step() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_.onboarding.step;
}

WifiConnectStatus StateStore::wifi_status() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_.onboarding.wifi_status;
}

OnboardingSubmission StateStore::pending_onboarding_submission() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return state_.onboarding.pending_submission;
}

void StateStore::set_screen(ScreenId screen) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.current_screen = screen;
    }
    bus_.publish(EventType::ScreenChanged, static_cast<int32_t>(screen));
}

void StateStore::set_boot_state(const BootState& boot) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.boot = boot;
    }
    bus_.publish(EventType::BootStateChanged);
}

void StateStore::request_boot_skip() {
    bus_.publish(EventType::BootSkipRequested);
}

void StateStore::set_clock(const ClockState& clock) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.clock = clock;
    }
    bus_.publish(EventType::ClockUpdated);
}

void StateStore::set_crypto(const CryptoSummary& crypto) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.crypto = crypto;
    }
    bus_.publish(EventType::MarketUpdated);
}

void StateStore::set_btc_ohlc(const OhlcSeries& ohlc) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        ohlc_ = ohlc;
    }
    bus_.publish(EventType::OhlcUpdated);
}

OhlcSeries StateStore::btc_ohlc() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return ohlc_;
}

void StateStore::set_forex(const ForexSummary& forex) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.forex = forex;
    }
    bus_.publish(EventType::ForexUpdated);
}

void StateStore::set_boot_diagnostics(const char* reset_reason, uint32_t reboot_count) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.system.reset_reason = reset_reason;
        state_.system.reboot_count = reboot_count;
    }
    bus_.publish(EventType::SystemStatusChanged);
}

void StateStore::set_weather(const WeatherSummary& weather) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.weather = weather;
    }
    bus_.publish(EventType::WeatherUpdated);
}

uint32_t StateStore::push_notification(const NotificationItem& notification) {
    uint32_t assigned_id = 0;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        NotificationItem stored = notification;
        uint32_t max_id = 0;
        for (const NotificationItem& item : state_.notifications.items) {
            if (item.id > max_id) {
                max_id = item.id;
            }
        }
        assigned_id = (stored.id != 0) ? stored.id : (max_id + 1);
        stored.id = assigned_id;
        state_.notifications.items.push_back(stored);
        sort_notifications(state_.notifications.items);
        if (state_.notifications.items.size() > kMaxNotifications) {
            state_.notifications.items.resize(kMaxNotifications);
        }

        uint32_t unread = 0;
        for (const NotificationItem& item : state_.notifications.items) {
            if (!item.read && item.level != NotificationLevel::Silent) {
                ++unread;
            }
        }
        state_.notifications.unread_count = unread;
    }
    bus_.publish(EventType::NotificationCreated, static_cast<int32_t>(assigned_id));
    return assigned_id;
}

void StateStore::mark_all_notifications_read() {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (NotificationItem& item : state_.notifications.items) {
            item.read = true;
        }
        state_.notifications.unread_count = 0;
    }
    bus_.publish(EventType::NotificationCreated, 0);
}

void StateStore::set_system_status(const SystemStatus& status) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.system = status;
    }
    bus_.publish(EventType::SystemStatusChanged);
}

void StateStore::submit_onboarding(const OnboardingSubmission& submission) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.onboarding.pending_submission = submission;
    }
    bus_.publish(EventType::OnboardingStepSubmitted, static_cast<int32_t>(submission.step));
}

void StateStore::request_wifi_scan() {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.onboarding.wifi_scan_status = WifiScanStatus::Scanning;
    }
    bus_.publish(EventType::WifiScanRequested);
}

void StateStore::set_onboarding_needed(bool needed) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.onboarding.needed = needed;
    }
    bus_.publish(EventType::OnboardingStateChanged);
}

void StateStore::set_onboarding_step(OnboardingStep step) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.onboarding.step = step;
    }
    bus_.publish(EventType::OnboardingStateChanged, static_cast<int32_t>(step));
}

void StateStore::set_wifi_status(WifiConnectStatus status) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.onboarding.wifi_status = status;
    }
    bus_.publish(EventType::OnboardingStateChanged);
}

void StateStore::set_preferences(const UserPreferences& preferences) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.preferences = preferences;
    }
    bus_.publish(EventType::PreferencesChanged);
    bus_.publish(EventType::OnboardingStateChanged);
}

void StateStore::set_wifi_networks(const std::vector<WifiNetwork>& networks,
                                   WifiScanStatus status) {
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        state_.onboarding.wifi_networks = networks;
        state_.onboarding.wifi_scan_status = status;
    }
    bus_.publish(EventType::OnboardingStateChanged);
}

}  // namespace nova
