#include "mock_board.hpp"

namespace nova {

bool MockBoard::init_display() {
    status_.display_ready = true;
    status_.touch_ready = true;
    return true;
}

bool MockBoard::start_network_transport() {
    status_.network_ready = false;
    return status_.network_ready;
}

BoardStatus MockBoard::bring_up() {
    status_ = BoardStatus{};
    init_display();
    status_.sd_ready = false;
    status_.board_ready = status_.display_ready;
    return status_;
}

bool MockBoard::lock_ui(uint32_t /*timeout_ms*/) {
    ui_locked_ = true;
    return true;
}

void MockBoard::unlock_ui() {
    ui_locked_ = false;
}

bool MockBoard::lock_shared_i2c(uint32_t timeout_ms) {
    shared_i2c_locked_ = lock_ui(timeout_ms);
    return shared_i2c_locked_;
}

void MockBoard::unlock_shared_i2c() {
    shared_i2c_locked_ = false;
    unlock_ui();
}

void MockBoard::set_brightness(int pct) {
    if (pct < 0) {
        brightness_pct_ = 0;
    } else if (pct > 100) {
        brightness_pct_ = 100;
    } else {
        brightness_pct_ = pct;
    }
}

bool MockBoard::wifi_scan_start() {
    if (!status_.network_ready) {
        return false;
    }
    wifi_scan_status_ = WifiScanStatus::Done;
    return true;
}

std::vector<WifiNetwork> MockBoard::wifi_take_scan_results() {
    wifi_scan_status_ = WifiScanStatus::Idle;
    return mock_scan_results_;
}

bool MockBoard::wifi_connect(const std::string& ssid, const std::string& /*password*/) {
    if (!status_.network_ready || ssid.empty() || wifi_connect_should_fail_) {
        return false;
    }
    pending_connect_event_ = WifiLinkEvent::Connected;
    return true;
}

WifiLinkEvent MockBoard::wifi_take_connect_event() {
    const WifiLinkEvent event = pending_connect_event_;
    pending_connect_event_ = WifiLinkEvent::None;
    return event;
}

}  // namespace nova
