#pragma once

#include "i_board.hpp"
#include "state_store.hpp"

#include <string>
#include <vector>

namespace nova {

class SetupService {
public:
    SetupService(StateStore& state_store, IBoard& board);

    void tick();
    static void tick_adapter(void* context);
    void request_wifi_scan();
    void set_onboarding_step(OnboardingStep step);
    void go_back();
    void submit_onboarding(const OnboardingSubmission& submission);
    // Loads persisted onboarding/preferences from storage; idempotent. Public
    // so app_main can load brightness/preferences before the first UI frame,
    // instead of waiting for the first tick().
    void load_from_storage();

private:
    void sync_transport_state();
    void maybe_start_initial_scan(uint32_t now_ms);
    void request_wifi_scan(uint32_t now_ms);
    void poll_wifi_scan(uint32_t now_ms);
    void poll_wifi_connect_event(uint32_t now_ms);
    bool try_wifi_scan();
    void complete_wifi_scan(std::vector<WifiNetwork> networks);
    void begin_wifi_connect();
    void schedule_wifi_reconnect(uint32_t now_ms);
    void handle_wifi_connected();
    void handle_wifi_disconnected(uint32_t now_ms);
    void publish_connect_state(WifiConnectStatus status, uint32_t reconnect_attempts);
    void publish_step(OnboardingStep step);
    void complete_onboarding();
    static uint32_t current_time_ms();

    StateStore& state_store_;
    IBoard& board_;
    bool initialized_{false};
    bool storage_loaded_{false};
    bool auto_scan_requested_{false};
    bool wifi_auto_reconnect_enabled_{false};
    bool wifi_connect_pending_{false};
    bool wifi_timeout_armed_{false};
    bool wifi_scan_pending_{false};
    bool wifi_scan_in_progress_{false};
    uint32_t wifi_retry_count_{0};
    uint32_t wifi_connect_started_ms_{0};
    uint32_t wifi_reconnect_due_ms_{0};
    uint32_t wifi_scan_started_ms_{0};
    uint32_t wifi_scan_retry_due_ms_{0};
    std::string saved_wifi_ssid_{};
    std::string saved_wifi_password_{};
};

}  // namespace nova
