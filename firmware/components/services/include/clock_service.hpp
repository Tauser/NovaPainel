// NovaPainel - services/clock_service.hpp
// Reads the system clock (time()/localtime_r) once per second. Implements the
// hybrid RTC↔NTP strategy (ADR-0009/0032, Fase 11):
//
//   - The Waveshare ESP32-P4-WIFI6-Touch-LCD-7B has the ESP32-P4's internal
//     RTC domain backed by a 1220 battery (docs/HARDWARE.md). While the
//     battery lives, time() returns a plausible value after a cold reboot —
//     no NTP needed to keep the clock running. Gate 14 (Fase 0) validates
//     this by power-cycling and comparing to NTP.
//   - When time() >= kMinPlausibleEpoch: clock_.synced = true (RTC or NTP).
//   - When time() < kMinPlausibleEpoch (dead/absent battery + no NTP yet):
//     a seeded mock clock advances at 1 tick/second so the UI always shows
//     something moving, with synced = false as the "not trustworthy" signal.
#pragma once

#include <string>

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
    void advance_mock();
    void apply_timezone_if_changed();

    StateStore& store_;
    ClockState  clock_{};
    uint32_t    last_tick_ms_{0};
    std::string applied_timezone_;
};

}  // namespace nova
