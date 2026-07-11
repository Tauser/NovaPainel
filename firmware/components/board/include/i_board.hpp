#pragma once

#include <cstdint>

namespace nova {

class IAudio {
public:
    virtual ~IAudio() = default;
};

struct BoardStatus {
    bool board_ready{false};
    bool display_ready{false};
    bool touch_ready{false};
    bool network_ready{false};
    bool sd_ready{false};
};

class IBoard {
public:
    virtual ~IBoard() = default;

    virtual const char* name() const = 0;
    virtual bool init_display() = 0;
    virtual BoardStatus bring_up() = 0;
    virtual BoardStatus status() const = 0;
    virtual bool lock_ui(uint32_t timeout_ms) = 0;
    virtual void unlock_ui() = 0;
    virtual bool lock_shared_i2c(uint32_t timeout_ms) = 0;
    virtual void unlock_shared_i2c() = 0;
    virtual void set_brightness(int pct) = 0;
    virtual IAudio* audio() = 0;
};

}  // namespace nova
