// NovaPainel - services/clock_service.hpp
// Mock clock. Advances a simulated wall-clock once per second and writes it to
// the StateStore (which emits ClockUpdated). The real ClockService will be fed
// by NTP and, if present, a hardware RTC (ADR-0009 / docs/HARDWARE.md).
#pragma once

#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class ClockService : public Service {
public:
    explicit ClockService(StateStore& store) : store_(store) {}

    const char* name() const override { return "ClockService"; }
    bool init() override;
    void tick(uint32_t now_ms) override;

private:
    StateStore& store_;
    ClockState  clock_{};
    uint32_t    last_tick_ms_{0};
};

}  // namespace nova
