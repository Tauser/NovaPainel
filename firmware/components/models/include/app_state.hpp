#pragma once

#include <cstdint>
#include <string>

namespace nova {

enum class ScreenId : uint8_t {
    Boot = 0,
    Home = 1,
    Placeholder = 2,
};

const char* to_string(ScreenId screen_id);

enum class DataSource : uint8_t {
    None,
    Live,
    Cache,
    Mock,
};

const char* to_string(DataSource source);

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
    UiState ui{};
};

}  // namespace nova
