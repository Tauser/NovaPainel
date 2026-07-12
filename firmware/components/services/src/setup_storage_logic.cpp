#include "setup_storage_logic.hpp"

namespace nova {

SetupStorageLoadResult resolve_persisted_setup(bool schema_present,
                                               uint16_t schema_version,
                                               const PersistedSetupData& persisted) {
    SetupStorageLoadResult result{};
    result.preferences = UserPreferences{};
    result.setup = SetupState{};
    result.setup.valid = true;
    result.setup.stale = false;
    result.setup.source = DataSource::Mock;
    result.setup.onboarding_required = true;
    result.setup.onboarding_step = OnboardingStep::Wifi;

    if (!schema_present) {
        result.should_initialize_schema = true;
        return result;
    }

    if (schema_version > kCurrentSetupSchemaVersion) {
        result.schema_supported = false;
        return result;
    }

    if (schema_version < kCurrentSetupSchemaVersion) {
        result.should_initialize_schema = true;
    }

    result.setup.source = DataSource::Cache;
    if (persisted.has_brightness_pct) {
        result.preferences.brightness_pct = persisted.brightness_pct;
    }
    if (persisted.has_use_24h) {
        result.preferences.use_24h = persisted.use_24h;
    }
    if (persisted.has_timezone && !persisted.timezone.empty()) {
        result.preferences.timezone = persisted.timezone;
    }
    if (persisted.has_onboarding_done) {
        result.setup.onboarding_required = !persisted.onboarding_done;
        result.setup.onboarding_step =
            persisted.onboarding_done ? OnboardingStep::Done : OnboardingStep::Wifi;
    }
    if (persisted.has_wifi_ssid && !persisted.wifi_ssid.empty()) {
        result.setup.wifi_configured = true;
        result.saved_wifi_ssid = persisted.wifi_ssid;
    }
    if (persisted.has_wifi_password) {
        result.saved_wifi_password = persisted.wifi_password;
    }

    return result;
}

}  // namespace nova
