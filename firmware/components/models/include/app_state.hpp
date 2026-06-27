// NovaPanel - models/app_state.hpp
// Central application state model. Plain data only.
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

enum class BootStage {
    InitializingHardware,
    LoadingSystem,
    VerifyingSensors,
    Ready,
};

struct BootState {
    uint8_t   progress_pct{0};
    BootStage stage{BootStage::InitializingHardware};
};

struct ClockState {
    int       year{1970};
    int       month{1};
    int       day{1};
    int       hour{0};
    int       minute{0};
    int       second{0};
    int       weekday{0};
    bool      synced{false};
    uint32_t  last_update_ms{0};
};

enum class DataSource { Live, Cache, Mock };

struct MarketSummary {
    double    btc_usd{0.0};
    double    btc_change_24h{0.0};
    bool      valid{false};
    bool      stale{false};
    DataSource source{DataSource::Mock};
    uint32_t  last_update_ms{0};

    double    usd_brl{0.0};
    bool      usd_brl_valid{false};
    bool      usd_brl_stale{false};
    DataSource usd_brl_source{DataSource::Mock};
    uint32_t  usd_brl_last_update_ms{0};
};

enum class WeatherCondition { Clear, Cloudy, Fog, Drizzle, Rain, Snow, Thunderstorm, Unknown };

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

struct SystemStatus {
    bool board_ready{false};
    bool display_ready{false};
    bool touch_ready{false};
    bool network_ready{false};
    bool sd_ready{false};
    bool cache_ready{false};
    const char* reset_reason{"?"};
    uint32_t    reboot_count{0};
};

enum class ThemeMode { Auto, Light, Dark };

struct UserPreferences {
    std::string display_name;
    std::string timezone;
    bool        time_format_24h{true};
    ThemeMode   theme{ThemeMode::Auto};
};

enum class OnboardingStep {
    DisplayName,
    Wifi,
    TimezoneAndFormat,
    Confirmation,
    Done,
};

enum class WifiConnectStatus { Idle, Connecting, Connected, Failed };

struct WifiNetwork {
    std::string ssid;
    int8_t      rssi{0};
    bool        secured{true};
};

enum class WifiScanStatus { Idle, Scanning, Done, Failed };

struct OnboardingSubmission {
    OnboardingStep step{OnboardingStep::DisplayName};
    std::string    display_name;
    std::string    wifi_ssid;
    std::string    wifi_password;
    std::string    timezone;
    bool           time_format_24h{true};
};

struct OnboardingState {
    bool                     needed{true};
    OnboardingStep           step{OnboardingStep::DisplayName};
    WifiConnectStatus        wifi_status{WifiConnectStatus::Idle};
    WifiScanStatus           wifi_scan_status{WifiScanStatus::Idle};
    std::vector<WifiNetwork> wifi_networks{};
    OnboardingSubmission     pending_submission{};
};

struct AppState {
    ScreenId        current_screen{ScreenId::Boot};
    BootState       boot{};
    ClockState      clock{};
    MarketSummary   market{};
    WeatherSummary  weather{};
    SystemStatus    system{};
    UserPreferences preferences{};
    OnboardingState onboarding{};
};

}  // namespace nova
