#include "clock_service.hpp"

#include <cstdlib>
#include <ctime>

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "ClockService";
constexpr time_t kMinPlausibleEpoch = 1700000000;

const char* posix_tz_for(const std::string& iana)
{
    if (iana == "America/Sao_Paulo")   return "BRT3";
    if (iana == "America/Manaus")      return "AMT4";
    if (iana == "America/Rio_Branco")  return "ACT5";
    if (iana == "America/Noronha")     return "FNT2";
    if (iana == "UTC")                 return "UTC0";
    return "BRT3";
}
}  // namespace

ClockService::ClockService(StateStore& store) : store_(store) {}

const char* ClockService::name() const
{
    return "ClockService";
}

bool ClockService::init()
{
    clock_ = ClockState{};
    clock_.year = 2026;
    clock_.month = 6;
    clock_.day = 25;
    clock_.hour = 12;
    clock_.minute = 0;
    clock_.second = 0;
    clock_.weekday = 3;
    clock_.synced = false;
    store_.set_clock(clock_);
    ESP_LOGI(kTag, "seeded mock time");
    return true;
}

void ClockService::apply_timezone_if_changed()
{
    const std::string wanted = store_.state().preferences.timezone;
    if (wanted == applied_timezone_) {
        return;
    }

    const char* tz = posix_tz_for(wanted);
    setenv("TZ", tz, 1);
    tzset();
    applied_timezone_ = wanted;
    ESP_LOGI(kTag, "timezone applied: '%s' -> %s", wanted.c_str(), tz);
}

void ClockService::advance_mock()
{
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

void ClockService::tick(uint32_t now_ms)
{
    if (last_tick_ms_ == 0) {
        last_tick_ms_ = now_ms;
    }
    if (now_ms - last_tick_ms_ < 1000u) {
        return;
    }
    last_tick_ms_ += 1000u;

    apply_timezone_if_changed();

    const time_t now = time(nullptr);
    if (now < kMinPlausibleEpoch) {
        advance_mock();
        clock_.synced = false;
    } else {
        struct tm local{};
        localtime_r(&now, &local);
        clock_.year = local.tm_year + 1900;
        clock_.month = local.tm_mon + 1;
        clock_.day = local.tm_mday;
        clock_.hour = local.tm_hour;
        clock_.minute = local.tm_min;
        clock_.second = local.tm_sec;
        clock_.weekday = local.tm_wday;
        clock_.synced = true;
    }

    if (clock_.synced != last_synced_) {
        ESP_LOGI(kTag, "clock source changed: %s",
                 clock_.synced ? "NTP/RTC plausible" : "local mock");
        last_synced_ = clock_.synced;
    }

    clock_.last_update_ms = now_ms;
    store_.set_clock(clock_);
}

}  // namespace nova
