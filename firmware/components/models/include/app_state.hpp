// NovaPainel - models/app_state.hpp
// Central application state model. Plain data only (no logic, no I/O) so it can
// be unit-tested on the host and shared across services and the UI layer.
#pragma once

#include <cstdint>

namespace nova {

enum class ScreenId {
    Boot,
    Home,
    Market,
    Calendar,
    Devices,
    Focus,
    PhotoFrame,
    Routines,
    Settings,
    System,
};

const char* to_string(ScreenId id);

// Wall-clock state. `synced` reflects whether NTP (or a future RTC) has provided
// a trustworthy time. See ADR-0009 / docs/HARDWARE.md for the offline caveat.
struct ClockState {
    int     year{1970};
    int     month{1};
    int     day{1};
    int     hour{0};
    int     minute{0};
    int     second{0};
    int     weekday{0};       // 0 = Sunday .. 6 = Saturday
    bool    synced{false};
    uint32_t last_update_ms{0};
};

// Origin of a market snapshot. Matches shared/schemas/market_summary.json "source".
enum class DataSource { Live, Cache, Mock };

// Market snapshot shown on Home / Market. Matches shared/schemas/market_summary.
struct MarketSummary {
    double     btc_usd{0.0};
    double     usd_brl{0.0};
    double     btc_change_24h{0.0};   // percent
    bool       valid{false};
    bool       stale{false};          // true when served from cache
    DataSource source{DataSource::Mock};
    uint32_t   last_update_ms{0};
};

// Hardware / runtime readiness flags. Filled by the Board + core during boot.
struct SystemStatus {
    bool board_ready{false};
    bool display_ready{false};
    bool touch_ready{false};
    bool network_ready{false};
    bool cache_ready{false};
};

struct AppState {
    ScreenId      current_screen{ScreenId::Boot};
    ClockState    clock{};
    MarketSummary market{};
    SystemStatus  system{};
};

}  // namespace nova
