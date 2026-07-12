#pragma once

#include "app_state.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace nova {

enum class ActionType : uint8_t {
    None,
    Navigate,
    Refresh,
    SetBrightness,
    SetupRequestWifiScan,
    SetupSetStep,
    SetupSubmit,
};

struct SetupSubmissionActionPayload {
    OnboardingStep step{OnboardingStep::Wifi};
    bool use_24h{true};
    char wifi_ssid[33]{};
    char wifi_password[65]{};
    char timezone[48]{};
};

struct Action {
    ActionType type{ActionType::None};
    int32_t value{0};
    SetupSubmissionActionPayload setup_submission{};
};

class ActionQueue {
public:
    using Handler = void (*)(const Action& action);

    // Shared between future UI publishers and main_loop drain. Protected by
    // queue_mutex_; overflow_count_ is a metric surfaced to StateStore.
    bool push(Action action);
    void drain(Handler handler);
    std::size_t size() const;
    uint32_t overflow_count() const;

private:
    static constexpr std::size_t kCapacity = 16;
    mutable std::mutex queue_mutex_{};
    std::array<Action, kCapacity> items_{};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t size_{0};
    uint32_t overflow_count_{0};
};

}  // namespace nova
