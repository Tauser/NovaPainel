// NovaPainel - board/mock_board.hpp
// Hardware abstraction layer (ADR-0003). The rest of the firmware talks to the
// IBoard interface and never to a concrete BSP. This lets us swap
// Waveshare / Elecrow / Makerfabs boards - or this mock - without touching core.
//
// MockBoard simulates a successful bring-up with logs only. No real BSP, no
// display/touch driver, no Wi-Fi (the real board needs the ESP32-C6 +
// ESP-Hosted link - see docs/HARDWARE.md, validated in Fase 0).
#pragma once

#include <cstdint>

namespace nova {

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

    // Performs (mock) hardware bring-up. Returns the resulting status.
    virtual BoardStatus bring_up() = 0;

    virtual BoardStatus status() const = 0;

    // LVGL is not thread-safe (ADR-0007/0013): exactly one "owner" task drives
    // it (the real board's internal LVGL port task; nothing, on the mock).
    // Any other code that touches LVGL objects (e.g. UiDispatcher's render
    // callback) must hold this lock first. Kept here, not in a separate LVGL
    // header, so IBoard stays free of LVGL/BSP types and app_main.cpp /
    // core/ remain host-checkable.
    virtual bool lock(uint32_t timeout_ms) = 0;
    virtual void unlock() = 0;
};

class MockBoard : public IBoard {
public:
    const char* name() const override { return "MockBoard"; }
    BoardStatus bring_up() override;
    BoardStatus status() const override { return status_; }

    // No real LVGL task to contend with in the mock.
    bool lock(uint32_t /*timeout_ms*/) override { return true; }
    void unlock() override {}

private:
    BoardStatus status_{};
};

}  // namespace nova
