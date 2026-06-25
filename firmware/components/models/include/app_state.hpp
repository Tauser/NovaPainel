// NovaPainel - models/app_state.hpp
// Central application state model. Plain data only (no logic, no I/O) so it can
// be unit-tested on the host and shared across services and the UI layer.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nova {

enum class ScreenId {
    Boot,
    Setup,
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
// usd_brl has its own valid/source/timestamp because it comes from a
// dedicated ForexProvider, fetched independently from btc_usd/btc_change_24h
// (CoinGeckoProvider) - one can be fresh while the other is stale/missing.
struct MarketSummary {
    double     btc_usd{0.0};
    double     btc_change_24h{0.0};   // percent
    bool       valid{false};
    bool       stale{false};          // true when served from cache
    DataSource source{DataSource::Mock};
    uint32_t   last_update_ms{0};

    double     usd_brl{0.0};
    bool       usd_brl_valid{false};
    bool       usd_brl_stale{false};
    DataSource usd_brl_source{DataSource::Mock};
    uint32_t   usd_brl_last_update_ms{0};
};

// Coarse bucket mapped from Open-Meteo's WMO weather code (see
// providers/open_meteo_provider.cpp for the mapping table) - good enough for
// an icon/label later; the MVP has no weather screen yet (docs/design/README.md).
enum class WeatherCondition { Clear, Cloudy, Fog, Drizzle, Rain, Snow, Thunderstorm, Unknown };

// Current weather snapshot. Location is fixed in OpenMeteoProvider for now
// (no location step in the wizard yet) - Brasília, DF.
struct WeatherSummary {
    double           temperature_c{0.0};
    double           feels_like_c{0.0};
    double           temp_max_c{0.0};
    double           temp_min_c{0.0};
    double           uv_index{0.0};
    int              humidity_pct{0};
    double           wind_speed_kmh{0.0};
    WeatherCondition condition{WeatherCondition::Unknown};
    bool             valid{false};
    bool             stale{false};
    DataSource       source{DataSource::Mock};
    uint32_t         last_update_ms{0};
};

// Hardware / runtime readiness flags. Filled by the Board + core during boot.
// sd_ready is the raw SDMMC card mount (board/hardware layer, Fase 3).
// cache_ready is the LittleFS cache abstraction on top of it (Fase 6,
// ADR-0015) - they are intentionally separate flags.
struct SystemStatus {
    bool board_ready{false};
    bool display_ready{false};
    bool touch_ready{false};
    bool network_ready{false};
    bool sd_ready{false};
    bool cache_ready{false};
};

// User-chosen preferences, collected by the onboarding wizard (ADR-0017) and
// persisted to NVS by SetupService. The same model is reused later by the
// Settings screen - editing a preference after onboarding is not a separate
// flow.
enum class ThemeMode { Auto, Light, Dark };

struct UserPreferences {
    std::string display_name;
    std::string timezone;            // e.g. "America/Sao_Paulo" - plain string in the MVP, no TZ database
    bool        time_format_24h{true};
    ThemeMode   theme{ThemeMode::Auto};
};

// Multi-step wizard sequence (ADR-0017/0023, 4 steps per
// docs/design/lvgl_export_reference/screens/screen_setup.c v2). `Done` means
// onboarding finished - AppState::onboarding.needed flips to false and the
// app moves on to Home. No Theme step - ThemeMode stays UserPreferences'
// default until a future Settings screen sets it.
enum class OnboardingStep {
    DisplayName,
    Wifi,
    TimezoneAndFormat,
    Confirmation,
    Done,
};

enum class WifiConnectStatus { Idle, Connecting, Connected, Failed };

// One scanned Wi-Fi AP, de-duplicated by SSID (strongest RSSI kept) and
// sorted strongest-first by SetupService before publishing.
struct WifiNetwork {
    std::string ssid;
    int8_t      rssi{0};
    bool        secured{true};
};

enum class WifiScanStatus { Idle, Scanning, Done, Failed };

// The "intention" the wizard UI publishes (ADR-0017: UI never persists or
// calls esp_wifi directly). Only the field(s) relevant to `step` are read by
// SetupService; the rest are left at their default.
struct OnboardingSubmission {
    OnboardingStep step{OnboardingStep::DisplayName};
    std::string    display_name;       // step == DisplayName
    std::string    wifi_ssid;          // step == Wifi
    std::string    wifi_password;      // step == Wifi
    std::string    timezone;           // step == TimezoneAndFormat
    bool           time_format_24h{true};  // step == TimezoneAndFormat
    // step == Confirmation carries no fields - it just tells SetupService
    // the user pressed "Iniciar NovaPanel" on the summary screen.
};

// Transient onboarding/wizard state. `pending_submission` is the last
// submission published via StateStore::submit_onboarding(), read once by
// SetupService and then acted on - it is not meant to be read by the UI.
struct OnboardingState {
    bool                      needed{true};  // true until SetupService confirms saved prefs exist (set in init())
    OnboardingStep            step{OnboardingStep::DisplayName};
    WifiConnectStatus         wifi_status{WifiConnectStatus::Idle};
    WifiScanStatus            wifi_scan_status{WifiScanStatus::Idle};
    std::vector<WifiNetwork>  wifi_networks{};
    OnboardingSubmission      pending_submission{};
};

struct AppState {
    ScreenId         current_screen{ScreenId::Boot};
    ClockState       clock{};
    MarketSummary    market{};
    WeatherSummary   weather{};
    SystemStatus     system{};
    UserPreferences  preferences{};
    OnboardingState  onboarding{};
};

}  // namespace nova
