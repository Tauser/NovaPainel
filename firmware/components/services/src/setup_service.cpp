#include "setup_service.hpp"

#include "setup_storage_logic.hpp"

#if defined(ESP_PLATFORM)
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#endif

namespace nova {
namespace {
constexpr uint32_t kWifiConnectTimeoutMs = 20000;
constexpr uint32_t kWifiReconnectDelayMs = 3000;
constexpr uint32_t kWifiScanRetryDelayMs = 1000;
constexpr uint32_t kWifiScanTimeoutMs = 15000;
constexpr uint32_t kMaxWifiRetries = 5;

#if defined(ESP_PLATFORM)
constexpr const char* kTag = "SetupService";
constexpr const char* kNamespace = "setup";
constexpr const char* kSchemaKey = "schema_v";
constexpr const char* kOnboardingDoneKey = "onboarding_done";
constexpr const char* kBrightnessKey = "brightness_pct";
constexpr const char* kUse24hKey = "use_24h";
constexpr const char* kTimezoneKey = "timezone";
constexpr const char* kWifiSsidKey = "wifi_ssid";
constexpr const char* kWifiPasswordKey = "wifi_pass";
#endif
}  // namespace

SetupService::SetupService(StateStore& state_store, IBoard& board)
    : state_store_(state_store), board_(board) {}

uint32_t SetupService::current_time_ms() {
#if defined(ESP_PLATFORM)
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
#else
    return 0;
#endif
}

void SetupService::load_from_storage() {
    if (storage_loaded_) {
        return;
    }
    storage_loaded_ = true;

    SetupStorageLoadResult resolved{};

#if defined(ESP_PLATFORM)
    nvs_handle_t handle{};
    PersistedSetupData persisted{};
    bool schema_present = false;
    uint16_t schema_version = 0;

    if (nvs_open(kNamespace, NVS_READWRITE, &handle) == ESP_OK) {
        const esp_err_t schema_err = nvs_get_u16(handle, kSchemaKey, &schema_version);
        schema_present = (schema_err == ESP_OK);

        uint8_t onboarding_done = 0;
        if (nvs_get_u8(handle, kOnboardingDoneKey, &onboarding_done) == ESP_OK) {
            persisted.has_onboarding_done = true;
            persisted.onboarding_done = onboarding_done != 0;
        }

        uint8_t brightness_pct = 0;
        if (nvs_get_u8(handle, kBrightnessKey, &brightness_pct) == ESP_OK) {
            persisted.has_brightness_pct = true;
            persisted.brightness_pct = brightness_pct;
        }

        uint8_t use_24h = 0;
        if (nvs_get_u8(handle, kUse24hKey, &use_24h) == ESP_OK) {
            persisted.has_use_24h = true;
            persisted.use_24h = use_24h != 0;
        }

        size_t timezone_len = 0;
        if (nvs_get_str(handle, kTimezoneKey, nullptr, &timezone_len) == ESP_OK && timezone_len > 0) {
            persisted.timezone.resize(timezone_len);
            if (nvs_get_str(handle, kTimezoneKey, persisted.timezone.data(), &timezone_len) == ESP_OK) {
                if (!persisted.timezone.empty() && persisted.timezone.back() == '\0') {
                    persisted.timezone.pop_back();
                }
                persisted.has_timezone = !persisted.timezone.empty();
            }
        }

        size_t wifi_ssid_len = 0;
        if (nvs_get_str(handle, kWifiSsidKey, nullptr, &wifi_ssid_len) == ESP_OK && wifi_ssid_len > 0) {
            persisted.wifi_ssid.resize(wifi_ssid_len);
            if (nvs_get_str(handle, kWifiSsidKey, persisted.wifi_ssid.data(), &wifi_ssid_len) == ESP_OK) {
                if (!persisted.wifi_ssid.empty() && persisted.wifi_ssid.back() == '\0') {
                    persisted.wifi_ssid.pop_back();
                }
                persisted.has_wifi_ssid = !persisted.wifi_ssid.empty();
            }
        }

        size_t wifi_password_len = 0;
        if (nvs_get_str(handle, kWifiPasswordKey, nullptr, &wifi_password_len) == ESP_OK &&
            wifi_password_len > 0) {
            persisted.wifi_password.resize(wifi_password_len);
            if (nvs_get_str(handle, kWifiPasswordKey, persisted.wifi_password.data(),
                            &wifi_password_len) == ESP_OK) {
                if (!persisted.wifi_password.empty() && persisted.wifi_password.back() == '\0') {
                    persisted.wifi_password.pop_back();
                }
                persisted.has_wifi_password = true;
            }
        }

        resolved = resolve_persisted_setup(schema_present, schema_version, persisted);

        if (resolved.should_initialize_schema && resolved.schema_supported) {
            if (nvs_set_u16(handle, kSchemaKey, kCurrentSetupSchemaVersion) == ESP_OK) {
                nvs_commit(handle);
            }
        }

        nvs_close(handle);
    } else {
        ESP_LOGW(kTag, "nvs_open failed; using runtime defaults");
        resolved = resolve_persisted_setup(false, 0, PersistedSetupData{});
    }
#else
    resolved = resolve_persisted_setup(false, 0, PersistedSetupData{});
#endif

    saved_wifi_ssid_ = resolved.saved_wifi_ssid;
    saved_wifi_password_ = resolved.saved_wifi_password;
    wifi_auto_reconnect_enabled_ = !saved_wifi_ssid_.empty();
    board_.set_brightness(resolved.preferences.brightness_pct);
    state_store_.set_preferences(resolved.preferences);
    state_store_.set_setup(resolved.setup);
}

void SetupService::sync_transport_state() {
    SetupState setup = state_store_.setup();
    const bool transport_ready = state_store_.system().network_ready;
    if (setup.transport_ready == transport_ready) {
        return;
    }

    const bool became_ready = transport_ready && !setup.transport_ready;
    setup.transport_ready = transport_ready;
    setup.valid = true;
    setup.stale = false;
    if (transport_ready) {
        setup.source = DataSource::Live;
    }
    setup.last_update_ms = current_time_ms();
    state_store_.set_setup(std::move(setup));

    if (became_ready && wifi_auto_reconnect_enabled_ && !wifi_connect_pending_ &&
        !wifi_timeout_armed_) {
        begin_wifi_connect();
    }
}

void SetupService::publish_step(OnboardingStep step) {
    SetupState setup = state_store_.setup();
    setup.onboarding_step = step;
    setup.last_update_ms = current_time_ms();
    state_store_.set_setup(std::move(setup));
}

void SetupService::publish_connect_state(WifiConnectStatus status, uint32_t reconnect_attempts) {
    SetupState setup = state_store_.setup();
    setup.valid = true;
    setup.stale = false;
    setup.transport_ready = state_store_.system().network_ready;
    if (setup.transport_ready) {
        setup.source = DataSource::Live;
    }
    setup.wifi_connect_status = status;
    setup.connect_in_progress = (status == WifiConnectStatus::Connecting);
    setup.reconnect_attempts = reconnect_attempts;
    setup.last_update_ms = current_time_ms();
    state_store_.set_setup(std::move(setup));
}

void SetupService::complete_onboarding() {
#if defined(ESP_PLATFORM)
    nvs_handle_t handle{};
    if (nvs_open(kNamespace, NVS_READWRITE, &handle) == ESP_OK) {
        if (nvs_set_u8(handle, kOnboardingDoneKey, 1) == ESP_OK) {
            (void)nvs_commit(handle);
        }
        nvs_close(handle);
    }
#endif
    SetupState setup = state_store_.setup();
    setup.onboarding_required = false;
    setup.onboarding_step = OnboardingStep::Done;
    setup.last_update_ms = current_time_ms();
    state_store_.set_setup(std::move(setup));
    state_store_.navigate_to(ScreenId::Home);
}

void SetupService::request_wifi_scan() {
    request_wifi_scan(current_time_ms());
}

void SetupService::set_onboarding_step(OnboardingStep step) {
    publish_step(step);
}

void SetupService::go_back() {
    switch (state_store_.setup().onboarding_step) {
        case OnboardingStep::TimezoneAndFormat:
            publish_step(OnboardingStep::Wifi);
            break;
        case OnboardingStep::Confirmation:
            publish_step(OnboardingStep::TimezoneAndFormat);
            break;
        case OnboardingStep::Wifi:
        case OnboardingStep::Done:
            break;
    }
}

void SetupService::submit_onboarding(const OnboardingSubmission& submission) {
#if defined(ESP_PLATFORM)
    nvs_handle_t handle{};
    const bool have_nvs = (nvs_open(kNamespace, NVS_READWRITE, &handle) == ESP_OK);
#endif

    switch (submission.step) {
        case OnboardingStep::Wifi: {
            if (!submission.wifi_ssid.empty()) {
                saved_wifi_ssid_ = submission.wifi_ssid;
                saved_wifi_password_ = submission.wifi_password;
#if defined(ESP_PLATFORM)
                if (have_nvs) {
                    (void)nvs_set_str(handle, kWifiSsidKey, saved_wifi_ssid_.c_str());
                    (void)nvs_set_str(handle, kWifiPasswordKey, saved_wifi_password_.c_str());
                    (void)nvs_commit(handle);
                }
#endif
                SetupState setup = state_store_.setup();
                setup.wifi_configured = true;
                setup.last_update_ms = current_time_ms();
                state_store_.set_setup(std::move(setup));
                wifi_auto_reconnect_enabled_ = true;
                wifi_retry_count_ = 0;
                // Only attempts immediately if the ESP-Hosted/C6 transport is
                // already up; sync_transport_state() retries once it becomes
                // ready, so onboarding never gets stuck if the wizard is
                // finished before the transport link comes up.
                begin_wifi_connect();
            }
            break;
        }
        case OnboardingStep::TimezoneAndFormat: {
            UserPreferences preferences = state_store_.preferences();
            preferences.timezone = submission.timezone.empty() ? preferences.timezone : submission.timezone;
            preferences.use_24h = submission.use_24h;
#if defined(ESP_PLATFORM)
            if (have_nvs) {
                (void)nvs_set_str(handle, kTimezoneKey, preferences.timezone.c_str());
                (void)nvs_set_u8(handle, kUse24hKey, preferences.use_24h ? 1 : 0);
                (void)nvs_commit(handle);
            }
#endif
            state_store_.set_preferences(std::move(preferences));
            publish_step(OnboardingStep::Confirmation);
            break;
        }
        case OnboardingStep::Confirmation:
            complete_onboarding();
            break;
        case OnboardingStep::Done:
            break;
    }

#if defined(ESP_PLATFORM)
    if (have_nvs) {
        nvs_close(handle);
    }
#endif
}

void SetupService::request_wifi_scan(uint32_t now_ms) {
    SetupState setup = state_store_.setup();
    setup.valid = true;
    setup.stale = false;
    setup.transport_ready = state_store_.system().network_ready;
    if (setup.transport_ready) {
        setup.source = DataSource::Live;
    }

    if (!setup.transport_ready) {
        setup.wifi_scan_status = WifiScanStatus::Failed;
        setup.scan_in_progress = false;
        setup.wifi_networks.clear();
        setup.last_update_ms = now_ms;
        state_store_.set_setup(std::move(setup));
        wifi_scan_pending_ = false;
        wifi_scan_in_progress_ = false;
        return;
    }

    setup.wifi_scan_status = WifiScanStatus::Scanning;
    setup.scan_in_progress = true;
    setup.last_update_ms = now_ms;
    state_store_.set_setup(std::move(setup));

    wifi_scan_pending_ = false;
    wifi_scan_started_ms_ = now_ms;
    if (try_wifi_scan()) {
        return;
    }

    wifi_scan_pending_ = true;
    wifi_scan_retry_due_ms_ = now_ms + kWifiScanRetryDelayMs;
}

void SetupService::maybe_start_initial_scan(uint32_t now_ms) {
    if (auto_scan_requested_) {
        return;
    }

    const SetupState setup = state_store_.setup();
    if (!setup.onboarding_required || setup.wifi_configured || !setup.transport_ready) {
        return;
    }

    auto_scan_requested_ = true;
    request_wifi_scan(now_ms);
}

bool SetupService::try_wifi_scan() {
    if (wifi_scan_in_progress_) {
        return true;
    }

    if (!board_.wifi_scan_start()) {
        return false;
    }

    wifi_scan_pending_ = false;
    wifi_scan_in_progress_ = true;
    return true;
}

void SetupService::poll_wifi_scan(uint32_t now_ms) {
    if (wifi_scan_pending_ && now_ms >= wifi_scan_retry_due_ms_) {
        if (try_wifi_scan()) {
            return;
        }

        const uint32_t scan_elapsed = now_ms - wifi_scan_started_ms_;
        if (scan_elapsed >= kWifiScanTimeoutMs) {
            wifi_scan_pending_ = false;
            SetupState setup = state_store_.setup();
            setup.wifi_scan_status = WifiScanStatus::Failed;
            setup.scan_in_progress = false;
            setup.last_update_ms = now_ms;
            state_store_.set_setup(std::move(setup));
        } else {
            wifi_scan_retry_due_ms_ = now_ms + kWifiScanRetryDelayMs;
        }
        return;
    }

    if (!wifi_scan_in_progress_) {
        return;
    }

    const WifiScanStatus board_status = board_.wifi_scan_poll();
    if (board_status == WifiScanStatus::Done) {
        complete_wifi_scan(board_.wifi_take_scan_results());
        return;
    }
    if (board_status == WifiScanStatus::Failed) {
        wifi_scan_in_progress_ = false;
        SetupState setup = state_store_.setup();
        setup.wifi_scan_status = WifiScanStatus::Failed;
        setup.scan_in_progress = false;
        setup.wifi_networks.clear();
        setup.last_update_ms = now_ms;
        state_store_.set_setup(std::move(setup));
        return;
    }

    const uint32_t scan_elapsed = now_ms - wifi_scan_started_ms_;
    if (scan_elapsed >= kWifiScanTimeoutMs) {
        wifi_scan_in_progress_ = false;
        SetupState setup = state_store_.setup();
        setup.wifi_scan_status = WifiScanStatus::Failed;
        setup.scan_in_progress = false;
        setup.wifi_networks.clear();
        setup.last_update_ms = now_ms;
        state_store_.set_setup(std::move(setup));
    }
}

void SetupService::complete_wifi_scan(std::vector<WifiNetwork> networks) {
    wifi_scan_in_progress_ = false;
    SetupState setup = state_store_.setup();
    setup.transport_ready = state_store_.system().network_ready;
    setup.wifi_scan_status = WifiScanStatus::Done;
    setup.scan_in_progress = false;
    setup.wifi_networks = std::move(networks);
    setup.last_update_ms = current_time_ms();
    if (setup.transport_ready) {
        setup.source = DataSource::Live;
    }
    state_store_.set_setup(std::move(setup));
}

void SetupService::handle_wifi_connected() {
    wifi_retry_count_ = 0;
    wifi_connect_pending_ = false;
    wifi_timeout_armed_ = false;
    publish_connect_state(WifiConnectStatus::Connected, 0);
    if (state_store_.setup().onboarding_step == OnboardingStep::Wifi) {
        publish_step(OnboardingStep::TimezoneAndFormat);
    }
}

void SetupService::handle_wifi_disconnected(uint32_t now_ms) {
    wifi_timeout_armed_ = false;
    if (!wifi_auto_reconnect_enabled_) {
        return;
    }

    const SetupState setup = state_store_.setup();
    if (setup.wifi_connect_status == WifiConnectStatus::Connected ||
        setup.wifi_connect_status == WifiConnectStatus::Connecting) {
        schedule_wifi_reconnect(now_ms);
    }
}

void SetupService::poll_wifi_connect_event(uint32_t now_ms) {
    const WifiLinkEvent event = board_.wifi_take_connect_event();
    if (event == WifiLinkEvent::Connected) {
        handle_wifi_connected();
    } else if (event == WifiLinkEvent::Disconnected) {
        handle_wifi_disconnected(now_ms);
    }
}

void SetupService::begin_wifi_connect() {
    if (saved_wifi_ssid_.empty()) {
        return;
    }
    if (!state_store_.system().network_ready) {
        // Transport not up yet (ESP-Hosted/C6 link not ready). Credentials
        // stay saved and wifi_auto_reconnect_enabled_ stays true;
        // sync_transport_state() retries once the transport becomes ready.
        return;
    }

    wifi_connect_pending_ = false;
    wifi_timeout_armed_ = true;
    wifi_connect_started_ms_ = current_time_ms();
    publish_connect_state(WifiConnectStatus::Connecting, wifi_retry_count_);

    if (!board_.wifi_connect(saved_wifi_ssid_, saved_wifi_password_)) {
#if defined(ESP_PLATFORM)
        ESP_LOGW(kTag, "board wifi_connect failed to start");
#endif
        wifi_timeout_armed_ = false;
        schedule_wifi_reconnect(wifi_connect_started_ms_);
    }
}

void SetupService::schedule_wifi_reconnect(uint32_t now_ms) {
    if (wifi_retry_count_ >= kMaxWifiRetries) {
#if defined(ESP_PLATFORM)
        ESP_LOGW(kTag, "Wi-Fi reconnect exhausted after %lu retries",
                 static_cast<unsigned long>(kMaxWifiRetries));
#endif
        wifi_connect_pending_ = false;
        wifi_timeout_armed_ = false;
        publish_connect_state(WifiConnectStatus::Failed, wifi_retry_count_);
        return;
    }

    ++wifi_retry_count_;
    wifi_connect_pending_ = true;
    wifi_timeout_armed_ = false;
    wifi_reconnect_due_ms_ = now_ms + kWifiReconnectDelayMs;
    publish_connect_state(WifiConnectStatus::Connecting, wifi_retry_count_);
}

void SetupService::tick() {
    if (!initialized_) {
        load_from_storage();
        initialized_ = true;
    }

    const uint32_t now_ms = current_time_ms();
    sync_transport_state();
    maybe_start_initial_scan(now_ms);
    poll_wifi_scan(now_ms);
    poll_wifi_connect_event(now_ms);

    if (wifi_connect_pending_ && now_ms >= wifi_reconnect_due_ms_) {
        begin_wifi_connect();
    }

    if (wifi_timeout_armed_) {
        const uint32_t elapsed = now_ms - wifi_connect_started_ms_;
        if (elapsed >= kWifiConnectTimeoutMs) {
            wifi_timeout_armed_ = false;
            schedule_wifi_reconnect(now_ms);
        }
    }
}

void SetupService::tick_adapter(void* context) {
    auto* service = static_cast<SetupService*>(context);
    if (service != nullptr) {
        service->tick();
    }
}

}  // namespace nova
