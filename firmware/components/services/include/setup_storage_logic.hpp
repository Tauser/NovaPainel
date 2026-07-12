#pragma once

#include "app_state.hpp"

#include <cstdint>
#include <string>

namespace nova {

inline constexpr uint16_t kCurrentSetupSchemaVersion = 1;

struct PersistedSetupData {
    bool has_onboarding_done{false};
    bool onboarding_done{false};
    bool has_brightness_pct{false};
    int brightness_pct{80};
    bool has_use_24h{false};
    bool use_24h{true};
    bool has_timezone{false};
    std::string timezone{};
    bool has_wifi_ssid{false};
    std::string wifi_ssid{};
    bool has_wifi_password{false};
    std::string wifi_password{};
};

struct SetupStorageLoadResult {
    bool schema_supported{true};
    bool should_initialize_schema{false};
    UserPreferences preferences{};
    SetupState setup{};
    std::string saved_wifi_ssid{};
    std::string saved_wifi_password{};
};

SetupStorageLoadResult resolve_persisted_setup(bool schema_present,
                                               uint16_t schema_version,
                                               const PersistedSetupData& persisted);

}  // namespace nova
