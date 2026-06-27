// NovaPanel - core/request_orchestrator.hpp
#pragma once

#include <cstdint>
#include <mutex>

namespace nova {

enum class RequestPriority {
    Critical,
    High,
    Normal,
    Low,
    Paused,
};

enum class DataDomain {
    Weather,
    MarketSummary,
    MarketRealtime,
    MarketCandles,
    Forex,
    Calendar,
    Notifications,
    Photos,
    System,
};

enum class CircuitState { Closed, Open, HalfOpen };

const char* to_string(DataDomain domain);
const char* to_string(RequestPriority priority);
const char* to_string(CircuitState state);

class RequestOrchestrator {
public:
    RequestOrchestrator();

    void set_focused_mode(bool enabled);
    bool focused_mode() const { return focused_mode_; }
    bool can_request(DataDomain domain) const;
    RequestPriority priority_for(DataDomain domain) const;
    uint32_t        interval_ms_for(DataDomain domain) const;
    bool            within_rate_limit(DataDomain domain) const;
    CircuitState    circuit_state(DataDomain domain) const;
    bool            is_degraded(DataDomain domain) const;
    void            note_request(DataDomain domain, uint32_t now_ms, bool success = true);
    void            update_clock(uint32_t now_ms);

private:
    struct Policy {
        bool            enabled;
        RequestPriority priority;
        uint32_t        interval_ms;
        uint32_t        max_per_minute;
    };

    struct Runtime {
        uint32_t     last_request_ms{0};
        uint32_t     window_start_ms{0};
        uint32_t     calls_in_window{0};
        CircuitState  circuit{CircuitState::Closed};
        uint32_t     consec_failures{0};
        uint32_t     open_until_ms{0};
        uint32_t     backoff_ms{0};
    };

    const Policy&  policy(DataDomain domain) const;
    Policy&        mutable_policy(DataDomain domain);
    Runtime&       runtime(DataDomain domain);
    const Runtime& runtime(DataDomain domain) const;
    void           open_circuit(DataDomain domain, uint32_t now_ms);
    uint32_t       jittered_backoff(uint32_t current_ms);

    static constexpr int      kDomainCount = 9;
    static constexpr uint32_t kFailureThreshold = 3;
    static constexpr uint32_t kBackoffInitialMs = 10u * 1000u;
    static constexpr uint32_t kBackoffMaxMs = 5u * 60u * 1000u;

    Policy   policies_[kDomainCount];
    Runtime  runtime_[kDomainCount];
    mutable std::recursive_mutex mutex_;
    bool     focused_mode_{false};
    uint32_t now_ms_{0};
    uint32_t prng_{0x9e3779b9u};
};

}  // namespace nova
