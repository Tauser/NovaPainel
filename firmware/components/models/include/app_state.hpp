#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nova {

enum class ScreenId : uint8_t {
    Boot = 0,
    Home = 1,
    Placeholder = 2,
    Setup = 3,
};

const char* to_string(ScreenId screen_id);

enum class DataSource : uint8_t {
    None,
    Live,
    Cache,
    Mock,
};

const char* to_string(DataSource source);

enum class WifiConnectStatus : uint8_t {
    Idle = 0,
    Connecting = 1,
    Connected = 2,
    Failed = 3,
};

const char* to_string(WifiConnectStatus status);

enum class WifiScanStatus : uint8_t {
    Idle = 0,
    Scanning = 1,
    Done = 2,
    Failed = 3,
};

const char* to_string(WifiScanStatus status);

struct WifiNetwork {
    std::string ssid{};
    int8_t rssi{0};
    bool secured{true};
};

enum class OnboardingStep : uint8_t {
    Wifi = 0,
    TimezoneAndFormat = 1,
    Confirmation = 2,
    Done = 3,
};

const char* to_string(OnboardingStep step);

struct OnboardingSubmission {
    OnboardingStep step{OnboardingStep::Wifi};
    std::string wifi_ssid{};
    std::string wifi_password{};
    std::string timezone{"America/Sao_Paulo"};
    bool use_24h{true};
};

struct DataStatus {
    bool valid{false};
    bool stale{true};
    DataSource source{DataSource::None};
    uint64_t last_update_ms{0};
};

struct ClockState : DataStatus {
    uint64_t unix_time_s{0};
    std::string timezone{"America/Sao_Paulo"};
};

struct MarketState : DataStatus {
    double btc_usd{0.0};
    double usd_brl{0.0};
};

struct WeatherState : DataStatus {
    double temperature_c{0.0};
    double precipitation_mm{0.0};
    std::string summary{};
};

struct SystemState : DataStatus {
    bool display_ready{false};
    bool network_ready{false};
    uint32_t action_queue_overflows{0};
};

struct UserPreferences {
    std::string timezone{"America/Sao_Paulo"};
    int brightness_pct{80};
    bool use_24h{true};
    // Sem seleção de local no onboarding ainda: default São Paulo, mesma
    // cidade do timezone default acima. WeatherService usa isto para
    // montar a query do Open-Meteo; persistência/UI de local ficam para
    // quando o onboarding aprender a coletar essa informação.
    double latitude{-23.5505};
    double longitude{-46.6333};
};

struct SetupState : DataStatus {
    bool onboarding_required{true};
    OnboardingStep onboarding_step{OnboardingStep::Wifi};
    bool transport_ready{false};
    bool wifi_configured{false};
    WifiConnectStatus wifi_connect_status{WifiConnectStatus::Idle};
    WifiScanStatus wifi_scan_status{WifiScanStatus::Idle};
    bool scan_in_progress{false};
    bool connect_in_progress{false};
    uint32_t reconnect_attempts{0};
    std::vector<WifiNetwork> wifi_networks{};
};

struct UiState {
    ScreenId active_screen{ScreenId::Boot};
    bool shell_ready{false};
    bool boot_complete{false};
    bool display_breadcrumb{false};
    uint32_t display_retry_count{0};
};

struct AppState {
    ClockState clock{};
    MarketState market{};
    WeatherState weather{};
    SystemState system{};
    UserPreferences preferences{};
    SetupState setup{};
    UiState ui{};
};

}  // namespace nova
