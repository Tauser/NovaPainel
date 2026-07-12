#include "shell_chrome_view_model.hpp"
#include "timezone_catalog.hpp"

#include <cstdio>

namespace nova {
namespace {

const char* network_text(bool network_ready) {
    return network_ready ? "rede ok" : "sem rede";
}

const char* clock_text(const AppState& state, char* buffer, std::size_t size) {
    if (!state.clock.valid || state.clock.unix_time_s == 0) {
        return "sem hora";
    }

    const int offset_minutes = find_timezone_offset_minutes(state.preferences.timezone);
    const int64_t local_seconds =
        static_cast<int64_t>(state.clock.unix_time_s) + static_cast<int64_t>(offset_minutes) * 60;
    int64_t day_seconds = local_seconds % 86400;
    if (day_seconds < 0) {
        day_seconds += 86400;
    }
    const unsigned hours = static_cast<unsigned>(day_seconds / 3600);
    const unsigned minutes = static_cast<unsigned>((day_seconds % 3600) / 60);
    std::snprintf(buffer, size, "%02u:%02u", hours, minutes);
    return buffer;
}

}  // namespace

ShellChromeViewModel make_shell_chrome_view_model(const AppState& state) {
    char time_buffer[8];
    char status_buffer[48];
    std::snprintf(status_buffer, sizeof(status_buffer),
                  "%s · %s",
                  clock_text(state, time_buffer, sizeof(time_buffer)),
                  network_text(state.system.network_ready));
    return ShellChromeViewModel{status_buffer};
}

}  // namespace nova
