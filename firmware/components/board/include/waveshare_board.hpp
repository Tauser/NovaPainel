#pragma once

#include "i_board.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace nova {

class WaveshareBoard : public IBoard {
public:
    const char* name() const override { return "WaveshareBoard"; }
    bool init_display() override;
    bool start_network_transport() override;
    void start_network_transport_async() override;
    BoardStatus bring_up() override;
    BoardStatus status() const override;
    bool lock_ui(uint32_t timeout_ms) override;
    void unlock_ui() override;
    bool lock_shared_i2c(uint32_t timeout_ms) override;
    void unlock_shared_i2c() override;
    void set_brightness(int pct) override;
    uint64_t rtc_unix_time_s() const override;
    IAudio* audio() override { return nullptr; }

    bool wifi_scan_start() override;
    WifiScanStatus wifi_scan_poll() override;
    std::vector<WifiNetwork> wifi_take_scan_results() override;
    bool wifi_connect(const std::string& ssid, const std::string& password) override;
    WifiLinkEvent wifi_take_connect_event() override;

    // Internal: invoked by the ESP-IDF event trampoline in waveshare_board.cpp.
    // `event_base` is an opaque esp_event_base_t (a `const char*`); kept as
    // const void* here so this header stays includable on the host build.
    void on_wifi_event(const void* event_base, int32_t event_id);

private:
    BoardStatus status_{};
    bool display_initialized_{false};
    std::atomic<bool> network_ready_{false};
    bool network_task_started_{false};
    bool wifi_events_registered_{false};

    std::atomic<bool> wifi_scan_done_pending_{false};
    WifiScanStatus wifi_scan_status_{WifiScanStatus::Idle};
    std::vector<WifiNetwork> wifi_scan_results_{};
    uint32_t wifi_scan_started_ms_{0};
    std::atomic<uint8_t> wifi_link_event_{0};
};

}  // namespace nova
