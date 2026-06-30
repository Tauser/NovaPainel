#pragma once

#include <atomic>

#include "event_bus.hpp"
#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class SetupService final : public Service {
public:
    SetupService(StateStore& store, EventBus& bus);

    const char* name() const override;
    bool init() override;
    void tick(uint32_t now_ms) override;

    void on_wifi_connected();
    void on_wifi_disconnected();
    void on_wifi_scan_done();

private:
    void drain_async_events(uint32_t now_ms);
    void handle_submission(const OnboardingSubmission& submission);
    void save_runtime_preferences(const UserPreferences& prefs);
    void handle_wifi_scan_request();
    bool try_wifi_scan();
    void complete_wifi_scan();
    void handle_wifi_connected();
    void handle_wifi_disconnected(uint32_t now_ms);
    void begin_wifi_connect();
    void schedule_wifi_reconnect(uint32_t now_ms);

    StateStore&    store_;
    EventBus&      bus_;
    SubscriptionId sub_id_{0};
    UserPreferences prefs_{};
    int wifi_retry_count_{0};
    bool wifi_auto_reconnect_enabled_{false};
    bool wifi_connect_pending_{false};
    bool wifi_timeout_armed_{false};
    bool wifi_scan_pending_{false};
    bool wifi_scan_in_progress_{false};
    std::atomic<bool> wifi_connected_pending_{false};
    std::atomic<bool> wifi_disconnected_pending_{false};
    std::atomic<bool> wifi_scan_done_pending_{false};
    uint32_t wifi_connect_started_ms_{0};
    uint32_t wifi_reconnect_due_ms_{0};
    uint32_t wifi_scan_started_ms_{0};
    uint32_t wifi_scan_retry_due_ms_{0};
};

}  // namespace nova
