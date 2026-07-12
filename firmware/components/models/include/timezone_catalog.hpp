#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace nova {

struct TimezoneOption {
    const char* name;
    const char* label;
    int utc_offset_minutes;
};

inline constexpr std::array<TimezoneOption, 5> kTimezoneOptions{{
    {"America/Sao_Paulo", "UTC-3 · Brasilia", -3 * 60},
    {"America/Manaus", "UTC-4 · Manaus", -4 * 60},
    {"America/Rio_Branco", "UTC-5 · Rio Branco", -5 * 60},
    {"America/Noronha", "UTC-2 · Noronha", -2 * 60},
    {"UTC", "UTC+0 · Universal", 0},
}};

inline constexpr std::size_t default_timezone_option_index() {
    return 0;
}

inline constexpr std::size_t timezone_option_count() {
    return kTimezoneOptions.size();
}

inline constexpr const TimezoneOption& timezone_option_at(std::size_t index) {
    return kTimezoneOptions[index];
}

inline constexpr std::size_t find_timezone_option_index(std::string_view timezone_name) {
    for (std::size_t index = 0; index < kTimezoneOptions.size(); ++index) {
        if (timezone_name == kTimezoneOptions[index].name) {
            return index;
        }
    }
    return default_timezone_option_index();
}

inline constexpr int find_timezone_offset_minutes(std::string_view timezone_name) {
    return kTimezoneOptions[find_timezone_option_index(timezone_name)].utc_offset_minutes;
}

}  // namespace nova
