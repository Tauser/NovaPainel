// NovaPainel - services/setup_service.hpp
// Real onboarding/Wi-Fi persistence (ADR-0017): NVS for preferences and Wi-Fi
// credentials, esp_wifi_set_config()/esp_wifi_connect() for the Wifi step.
// SetupService is the ONLY thing allowed to persist or touch Wi-Fi directly -
// the wizard UI only publishes an intention via StateStore::submit_onboarding().
//
// Kept free of ESP-IDF/NVS/Wi-Fi types here on purpose (mirrors
// waveshare_board.hpp) - only setup_service.cpp pulls in the real headers, so
// app_main.cpp stays host-checkable even though setup_service.cpp itself is
// hardware-only (see tools/scripts/host_check.sh).
#pragma once

#include "service.hpp"
#include "state_store.hpp"
#include "event_bus.hpp"

namespace nova {

class SetupService : public Service {
public:
    SetupService(StateStore& store, EventBus& bus) : store_(store), bus_(bus) {}

    const char* name() const override { return "SetupService"; }

    // Loads saved preferences + the onboarding-done flag from NVS, subscribes
    // to OnboardingStepSubmitted, and registers the Wi-Fi event handlers.
    bool init() override;
    void tick(uint32_t now_ms) override;

    // Called by the free wifi_event_handler() trampoline in setup_service.cpp
    // (kept public, not a callback type, so the header stays ESP-IDF-free).
    void on_wifi_connected();
    void on_wifi_disconnected();

private:
    void handle_submission(const OnboardingSubmission& submission);
    void handle_wifi_scan_request();
    void begin_wifi_connect();
    void schedule_wifi_reconnect(uint32_t now_ms);

    StateStore&     store_;
    EventBus&       bus_;
    SubscriptionId  sub_id_{0};
    UserPreferences prefs_{};
    int wifi_retry_count_{0};
    bool wifi_auto_reconnect_enabled_{false};
    bool wifi_connect_pending_{false};
    bool wifi_timeout_armed_{false};
    uint32_t wifi_connect_started_ms_{0};
    uint32_t wifi_reconnect_due_ms_{0};
};

}  // namespace nova
