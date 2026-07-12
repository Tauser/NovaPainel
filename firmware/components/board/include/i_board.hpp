#pragma once

#include "app_state.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace nova {

class IAudio {
public:
    virtual ~IAudio() = default;
};

struct BoardStatus {
    bool board_ready{false};
    bool display_ready{false};
    bool touch_ready{false};
    bool network_ready{false};
    bool sd_ready{false};
};

// Edge-triggered outcome of an in-flight Wi-Fi station connection attempt,
// consumed once via IBoard::wifi_take_connect_event().
enum class WifiLinkEvent : uint8_t {
    None = 0,
    Connected = 1,
    Disconnected = 2,
};

class IBoard {
public:
    virtual ~IBoard() = default;

    virtual const char* name() const = 0;
    virtual bool init_display() = 0;
    // Blocking bring-up of the network transport (ESP-Hosted/C6 link etc).
    // May take several seconds; callers on the main boot path should prefer
    // start_network_transport_async().
    virtual bool start_network_transport() = 0;
    // Kicks off network transport bring-up without blocking the caller.
    // status().network_ready reflects the outcome once it completes.
    virtual void start_network_transport_async() = 0;
    virtual BoardStatus bring_up() = 0;
    virtual BoardStatus status() const = 0;
    virtual bool lock_ui(uint32_t timeout_ms) = 0;
    virtual void unlock_ui() = 0;
    virtual bool lock_shared_i2c(uint32_t timeout_ms) = 0;
    virtual void unlock_shared_i2c() = 0;
    virtual void set_brightness(int pct) = 0;
    virtual uint64_t rtc_unix_time_s() const = 0;
    virtual IAudio* audio() = 0;

    // Wi-Fi station control. Only meaningful once status().network_ready is
    // true; implementations are free to no-op/fail otherwise. All of this
    // lives behind IBoard per CLAUDE.md rule 5 ("Hardware SO atras de
    // board/") so services/ never touches esp_wifi directly.
    virtual bool wifi_scan_start() = 0;
    virtual WifiScanStatus wifi_scan_poll() = 0;
    virtual std::vector<WifiNetwork> wifi_take_scan_results() = 0;
    virtual bool wifi_connect(const std::string& ssid, const std::string& password) = 0;
    virtual WifiLinkEvent wifi_take_connect_event() = 0;
};

}  // namespace nova
