// NovaPainel - services/clock_service.hpp
// Reads the system clock (time()/localtime_r) once per second. Before NTP
// has synced it (SetupService starts SNTP on Wi-Fi connect, ADR-0021), the
// system clock reads close to the Unix epoch - this service detects that and
// falls back to advancing a seeded mock clock instead, so the UI always has
// something reasonable to show (ADR-0009 offline caveat). No hardware RTC
// yet (Fase 0 risk, docs/HARDWARE.md) - epoch-since-boot is all there is
// until NTP fixes it.
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
