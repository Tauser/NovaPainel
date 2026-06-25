// NovaPainel - services/clock_service.cpp
#include "clock_service.hpp"

#include <cstdlib>
#include <ctime>

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "ClockService";

// Below this, the system clock hasn't been NTP-synced yet (it's still near
// the Unix epoch - there's no battery-backed RTC on this board, Fase 0).
// Picked well in the past relative to this project's existence, not tied to
// "now" - just needs to be far enough past 1970 to never false-positive.
constexpr time_t kMinPlausibleEpoch = 1700000000;  // 2023-11-14

// Maps the wizard's curated IANA-style strings (app_state.hpp) to a POSIX TZ
// rule string newlib's tzset()/localtime() understands - there is no IANA
// tzdata bundled on the device, so this hand-mapping is the MVP's whole
// timezone database. All 5 entries match the wizard's dropdown 1:1; keep
// them in sync if that list changes.
const char* posix_tz_for(const std::string& iana) {
    if (iana == "America/Sao_Paulo")   return "BRT3";
    if (iana == "America/Manaus")      return "AMT4";
    if (iana == "America/Rio_Branco")  return "ACT5";
    if (iana == "America/Noronha")     return "FNT2";
    if (iana == "UTC")                 return "UTC0";
    return "BRT3";  // default before onboarding picks one - matches the dropdown's first option
}
}  // namespace

bool ClockService::init() {
    // Seed a mock time so the UI shows something moving even before NTP
    // syncs (ADR-0009). Real date/time take over in tick() once time()
    // reports a plausible epoch.
    clock_ = ClockState{};
    clock_.year = 2026; clock_.month = 6; clock_.day = 14;
    clock_.hour = 22;   clock_.minute = 41; clock_.second = 0;
    clock_.weekday = 0;  // Sunday
    clock_.synced = false;
    store_.set_clock(clock_);
    ESP_LOGI(kTag, "seeded mock time 22:41:00 (not synced)");
    return true;
}

void ClockService::apply_timezone_if_changed() {
    const std::string& wanted = store_.state().preferences.timezone;
    if (wanted == applied_timezone_) return;
    setenv("TZ", posix_tz_for(wanted), 1);
    tzset();
    applied_timezone_ = wanted;
    ESP_LOGI(kTag, "timezone applied: '%s' -> %s", wanted.c_str(), posix_tz_for(wanted));
}

void ClockService::advance_mock() {
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
}

void ClockService::tick(uint32_t now_ms) {
    if (last_tick_ms_ == 0) last_tick_ms_ = now_ms;
    if (now_ms - last_tick_ms_ < 1000u) return;  // advance once per second
    last_tick_ms_ += 1000u;

    apply_timezone_if_changed();

    const time_t now = time(nullptr);
    if (now < kMinPlausibleEpoch) {
        advance_mock();
        clock_.synced = false;
    } else {
        struct tm local{};
        localtime_r(&now, &local);
        clock_.year    = local.tm_year + 1900;
        clock_.month   = local.tm_mon + 1;
        clock_.day     = local.tm_mday;
        clock_.hour    = local.tm_hour;
        clock_.minute  = local.tm_min;
        clock_.second  = local.tm_sec;
        clock_.weekday = local.tm_wday;
        clock_.synced  = true;
    }

    clock_.last_update_ms = now_ms;
    store_.set_clock(clock_);
}

}  // namespace nova
