#pragma once

#include "i_board.hpp"

namespace nova {

class WaveshareBoard : public IBoard {
public:
    const char* name() const override { return "WaveshareBoard"; }
    bool init_display() override;
    BoardStatus bring_up() override;
    BoardStatus status() const override { return status_; }
    bool lock_ui(uint32_t timeout_ms) override;
    void unlock_ui() override;
    bool lock_shared_i2c(uint32_t timeout_ms) override;
    void unlock_shared_i2c() override;
    void set_brightness(int pct) override;
    IAudio* audio() override { return nullptr; }

private:
    BoardStatus status_{};
    bool display_initialized_{false};
};

}  // namespace nova
