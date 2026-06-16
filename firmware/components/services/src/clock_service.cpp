// NovaPainel - services/clock_service.cpp
#include "clock_service.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "ClockService";
}

bool ClockService::init() {
    // Seed a mock time (Sunday 22:41:00, 2026-06-14). `synced=false` because no
    // NTP/RTC has confirmed it yet.
    clock_ = ClockState{};
    clock_.year = 2026; clock_.month = 6; clock_.day = 14;
    clock_.hour = 22;   clock_.minute = 41; clock_.second = 0;
    clock_.weekday = 0;  // Sunday
    clock_.synced = false;
    store_.set_clock(clock_);
    ESP_LOGI(kTag, "seeded mock time 22:41:00 (not synced)");
    return true;
}

void ClockService::tick(uint32_t now_ms) {
    if (last_tick_ms_ == 0) last_tick_ms_ = now_ms;
    if (now_ms - last_tick_ms_ < 1000u) return;  // advance once per second
    last_tick_ms_ += 1000u;

    if (++clock_.second >= 60) {
        clock_.second = 0;
        if (++clock_.minute >= 60) {
            clock_.minute = 0;
            if (++clock_.hour >= 24) {
                clock_.hour = 0;
                ++clock_.day;
                clock_.weekday = (clock_.weekday + 1) % 7;
            }
        }
    }
    clock_.last_update_ms = now_ms;
    store_.set_clock(clock_);
}

}  // namespace nova
