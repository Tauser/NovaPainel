#pragma once

#include "status.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace nova {

enum class RequestDomain : uint8_t {
    Weather,
    MarketSpot,
    Forex,
    Ohlc,
};

enum class RequestPriority : uint8_t {
    Low,
    Normal,
    Critical,
};

enum class CircuitState : uint8_t {
    Closed,
    Open,
    HalfOpen,
};

struct RequestPolicy {
    RequestDomain domain{RequestDomain::Weather};
    RequestPriority priority{RequestPriority::Normal};
    uint32_t min_interval_ms{0};
    uint32_t rate_limit_per_minute{6};
    uint32_t circuit_failures_to_open{3};
    uint32_t circuit_base_backoff_ms{30000};
    uint32_t circuit_max_backoff_ms{30 * 60 * 1000};
    bool enabled{true};
};

class RequestOrchestrator {
public:
    RequestOrchestrator();

    Status configure(RequestPolicy policy);
    Status configure_default_policies();
    bool can_start(RequestDomain domain, uint64_t now_ms) const;
    Status mark_started(RequestDomain domain, uint64_t now_ms);
    void mark_finished(RequestDomain domain, bool ok, uint64_t now_ms);
    void tick(uint64_t now_ms);
    CircuitState circuit_state(RequestDomain domain) const;
    uint32_t consecutive_failures(RequestDomain domain) const;
    bool in_flight() const { return in_flight_; }

private:
    struct DomainState {
        RequestPolicy policy{};
        uint64_t last_start_ms{0};
        uint64_t rate_window_start_ms{0};
        uint32_t starts_in_window{0};
        uint32_t consecutive_failures{0};
        CircuitState circuit_state{CircuitState::Closed};
        uint64_t opened_at_ms{0};
        uint32_t current_backoff_ms{0};
        bool configured{false};
    };

    DomainState* find(RequestDomain domain);
    const DomainState* find(RequestDomain domain) const;
    static uint32_t next_backoff_ms(const DomainState& state);
    static bool rate_limited(const DomainState& state, uint64_t now_ms);
    static bool circuit_allows_start(const DomainState& state, uint64_t now_ms);

    static constexpr std::size_t kMaxDomains = 8;
    static constexpr uint32_t kGlobalGapMs = 400;
    std::array<DomainState, kMaxDomains> domains_{};
    RequestDomain in_flight_domain_{RequestDomain::Weather};
    uint64_t last_global_start_ms_{0};
    bool in_flight_{false};
};

}  // namespace nova
