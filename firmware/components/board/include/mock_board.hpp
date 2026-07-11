#pragma once

#include "i_board.hpp"

namespace nova {

class MockBoard : public IBoard {
public:
    const char* name() const override { return "MockBoard"; }
    bool init_display() override;
    BoardStatus bring_up() override;
    BoardStatus status() const override { return status_; }
    bool lock_ui(uint32_t timeout_ms) override;
    void unlock_ui() override;
    bool lock_shared_i2c(uint32_t timeout_ms) override;
    void unlock_shared_i2c() override;
    void set_brightness(int pct) override;
    IAudio* audio() override { return nullptr; }

    int brightness_pct() const { return brightness_pct_; }

private:
    BoardStatus status_{};
    int brightness_pct_{80};
    bool ui_locked_{false};
    bool shared_i2c_locked_{false};
};

}  // namespace nova
