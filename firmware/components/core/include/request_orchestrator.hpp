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

struct RequestPolicy {
    RequestDomain domain{RequestDomain::Weather};
    RequestPriority priority{RequestPriority::Normal};
    uint32_t min_interval_ms{0};
    uint32_t rate_limit_per_minute{6};
    bool enabled{true};
};

class RequestOrchestrator {
public:
    Status configure(RequestPolicy policy);
    bool can_start(RequestDomain domain, uint64_t now_ms) const;
    Status mark_started(RequestDomain domain, uint64_t now_ms);
    void mark_finished(RequestDomain domain, bool ok, uint64_t now_ms);
    void tick(uint64_t now_ms);

private:
    struct DomainState {
        RequestPolicy policy{};
        uint64_t last_start_ms{0};
        uint32_t consecutive_failures{0};
        bool configured{false};
    };

    DomainState* find(RequestDomain domain);
    const DomainState* find(RequestDomain domain) const;

    static constexpr std::size_t kMaxDomains = 8;
    std::array<DomainState, kMaxDomains> domains_{};
};

}  // namespace nova
