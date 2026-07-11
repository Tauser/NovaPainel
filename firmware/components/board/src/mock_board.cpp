#include "mock_board.hpp"

namespace nova {

bool MockBoard::init_display() {
    status_.display_ready = true;
    status_.touch_ready = true;
    return true;
}

BoardStatus MockBoard::bring_up() {
    status_ = BoardStatus{};
    init_display();
    status_.network_ready = false;
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

}  // namespace nova
