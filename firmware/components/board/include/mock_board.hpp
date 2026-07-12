#pragma once

#include "i_board.hpp"

#include <string>
#include <vector>

namespace nova {

class MockBoard : public IBoard {
public:
    const char* name() const override { return "MockBoard"; }
    bool init_display() override;
    bool start_network_transport() override;
    void start_network_transport_async() override { status_.network_ready = start_network_transport(); }
    BoardStatus bring_up() override;
    BoardStatus status() const override { return status_; }
    bool lock_ui(uint32_t timeout_ms) override;
    void unlock_ui() override;
    bool lock_shared_i2c(uint32_t timeout_ms) override;
    void unlock_shared_i2c() override;
    void set_brightness(int pct) override;
    uint64_t rtc_unix_time_s() const override { return rtc_unix_time_s_; }
    IAudio* audio() override { return nullptr; }

    bool wifi_scan_start() override;
    WifiScanStatus wifi_scan_poll() override { return wifi_scan_status_; }
    std::vector<WifiNetwork> wifi_take_scan_results() override;
    bool wifi_connect(const std::string& ssid, const std::string& password) override;
    WifiLinkEvent wifi_take_connect_event() override;

    int brightness_pct() const { return brightness_pct_; }
    void set_rtc_unix_time_s(uint64_t unix_time_s) { rtc_unix_time_s_ = unix_time_s; }
    void set_network_ready(bool ready) { status_.network_ready = ready; }
    void set_wifi_scan_results(std::vector<WifiNetwork> networks) { mock_scan_results_ = std::move(networks); }
    void set_wifi_connect_should_fail(bool should_fail) { wifi_connect_should_fail_ = should_fail; }

private:
    BoardStatus status_{};
    int brightness_pct_{80};
    bool ui_locked_{false};
    bool shared_i2c_locked_{false};
    uint64_t rtc_unix_time_s_{0};
    WifiScanStatus wifi_scan_status_{WifiScanStatus::Idle};
    std::vector<WifiNetwork> mock_scan_results_{};
    bool wifi_connect_should_fail_{false};
    WifiLinkEvent pending_connect_event_{WifiLinkEvent::None};
};

}  // namespace nova
