#include "setup_service.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace nova {

namespace {
constexpr const char* kTag = "SetupService";
constexpr const char* kNvsNamespace = "novapanel";

constexpr const char* kKeyOnboardingDone = "onb_done";
constexpr const char* kKeySchemaVersion = "schema_v";
constexpr const char* kKeyDisplayName = "disp_name";
constexpr const char* kKeyTimezone = "timezone";
constexpr const char* kKeyTimeFormat24h = "time_24h";
constexpr const char* kKeyTheme = "theme";
constexpr const char* kKeyWifiSsid = "wifi_ssid";
constexpr const char* kKeyWifiPassword = "wifi_pass";

constexpr uint16_t kCurrentNvsSchemaVersion = 1;
constexpr uint32_t kWifiConnectTimeoutMs = 20000;
constexpr uint32_t kWifiReconnectDelayMs = 3000;
constexpr uint32_t kWifiScanRetryDelayMs = 1000;
constexpr uint32_t kWifiScanTimeoutMs = 15000;
constexpr int kMaxWifiRetries = 5;
constexpr uint16_t kMaxScanResults = 12;

bool read_string(nvs_handle_t handle, const char* key, std::string& out)
{
    size_t len = 0;
    if (nvs_get_str(handle, key, nullptr, &len) != ESP_OK || len == 0) {
        return false;
    }
    out.resize(len);
    if (nvs_get_str(handle, key, out.data(), &len) != ESP_OK) {
        return false;
    }
    while (!out.empty() && out.back() == '\0') {
        out.pop_back();
    }
    return true;
}

bool key_exists(nvs_handle_t handle, const char* key)
{
    nvs_type_t type = NVS_TYPE_ANY;
    return nvs_find_key(handle, key, &type) == ESP_OK;
}

bool migrate_or_validate_schema(nvs_handle_t handle, bool& schema_supported)
{
    schema_supported = false;

    uint16_t schema_version = 0;
    const esp_err_t err = nvs_get_u16(handle, kKeySchemaVersion, &schema_version);
    if (err == ESP_OK) {
        if (schema_version > kCurrentNvsSchemaVersion) {
            ESP_LOGW(kTag, "NVS schema v%u newer than firmware v%u; ignoring persisted setup",
                     static_cast<unsigned>(schema_version),
                     static_cast<unsigned>(kCurrentNvsSchemaVersion));
            return true;
        }
        schema_supported = true;
        return true;
    }

    if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(kTag, "nvs_get_u16(schema_v) failed: %s", esp_err_to_name(err));
        return false;
    }

    const bool has_legacy_data =
        key_exists(handle, kKeyOnboardingDone) ||
        key_exists(handle, kKeyDisplayName) ||
        key_exists(handle, kKeyTimezone) ||
        key_exists(handle, kKeyTimeFormat24h) ||
        key_exists(handle, kKeyTheme) ||
        key_exists(handle, kKeyWifiSsid) ||
        key_exists(handle, kKeyWifiPassword);

    if (!has_legacy_data) {
        if (nvs_set_u16(handle, kKeySchemaVersion, kCurrentNvsSchemaVersion) != ESP_OK ||
            nvs_commit(handle) != ESP_OK) {
            ESP_LOGW(kTag, "failed to initialize empty NVS schema_v");
            return false;
        }
        schema_supported = true;
        return true;
    }

    if (nvs_set_u16(handle, kKeySchemaVersion, kCurrentNvsSchemaVersion) != ESP_OK ||
        nvs_commit(handle) != ESP_OK) {
        ESP_LOGW(kTag, "failed to migrate legacy NVS schema");
        return false;
    }

    ESP_LOGI(kTag, "migrated legacy NVS layout to schema v%u",
             static_cast<unsigned>(kCurrentNvsSchemaVersion));
    schema_supported = true;
    return true;
}

void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void*)
{
    auto* self = static_cast<SetupService*>(arg);
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        self->on_wifi_disconnected();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE) {
        self->on_wifi_scan_done();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        self->on_wifi_connected();
    }
}

}  // namespace

SetupService::SetupService(StateStore& store, EventBus& bus)
    : store_(store), bus_(bus)
{
}

const char* SetupService::name() const
{
    return "SetupService";
}

bool SetupService::init()
{
    nvs_flash_init();

    nvs_handle_t handle{};
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK) {
        ESP_LOGE(kTag, "nvs_open failed");
        return false;
    }

    bool schema_supported = false;
    if (!migrate_or_validate_schema(handle, schema_supported)) {
        nvs_close(handle);
        return false;
    }

    uint8_t onboarding_done = 0;
    if (schema_supported) {
        nvs_get_u8(handle, kKeyOnboardingDone, &onboarding_done);
        read_string(handle, kKeyDisplayName, prefs_.display_name);
        read_string(handle, kKeyTimezone, prefs_.timezone);
    }

    uint8_t fmt24h = 1;
    if (schema_supported) {
        nvs_get_u8(handle, kKeyTimeFormat24h, &fmt24h);
    }
    prefs_.time_format_24h = (fmt24h != 0);

    uint8_t theme = static_cast<uint8_t>(ThemeMode::Auto);
    if (schema_supported) {
        nvs_get_u8(handle, kKeyTheme, &theme);
    }
    prefs_.theme = static_cast<ThemeMode>(theme);

    std::string saved_ssid;
    std::string saved_password;
    const bool have_saved_wifi = schema_supported && onboarding_done != 0 &&
        read_string(handle, kKeyWifiSsid, saved_ssid) && !saved_ssid.empty();
    if (have_saved_wifi) {
        read_string(handle, kKeyWifiPassword, saved_password);
    }

    nvs_close(handle);

    store_.set_preferences(prefs_);
    store_.set_onboarding_needed(!schema_supported || onboarding_done == 0);
    store_.set_onboarding_step(onboarding_done == 0
        ? OnboardingStep::DisplayName
        : OnboardingStep::Done);

    sub_id_ = bus_.subscribe([this](const Event& e) {
        if (e.type == EventType::OnboardingStepSubmitted) {
            handle_submission(store_.snapshot().onboarding.pending_submission);
        } else if (e.type == EventType::WifiScanRequested) {
            handle_wifi_scan_request();
        }
    });

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, this, nullptr);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler, this, nullptr);

    if (have_saved_wifi) {
        wifi_config_t wifi_config{};
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid),
                     saved_ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password),
                     saved_password.c_str(), sizeof(wifi_config.sta.password) - 1);

        wifi_auto_reconnect_enabled_ = true;
        store_.set_wifi_status(WifiConnectStatus::Connecting);
        if (esp_wifi_set_config(WIFI_IF_STA, &wifi_config) != ESP_OK) {
            ESP_LOGW(kTag, "auto-reconnect esp_wifi_set_config failed");
            wifi_auto_reconnect_enabled_ = false;
            store_.set_wifi_status(WifiConnectStatus::Failed);
        } else {
            wifi_retry_count_ = 0;
            begin_wifi_connect();
        }
    }

    esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    sntp_config.start = false;
    sntp_config.wait_for_sync = false;
    esp_netif_sntp_init(&sntp_config);

    ESP_LOGI(kTag, "onboarding_needed=%d display_name='%s'",
             onboarding_done == 0, prefs_.display_name.c_str());
    return true;
}

void SetupService::tick(uint32_t now_ms)
{
    if (wifi_connect_pending_ && now_ms >= wifi_reconnect_due_ms_) {
        begin_wifi_connect();
    }

    if (wifi_scan_pending_ && now_ms >= wifi_scan_retry_due_ms_) {
        if (try_wifi_scan()) {
            return;
        }

        const uint32_t scan_elapsed = now_ms - wifi_scan_started_ms_;
        if (scan_elapsed >= kWifiScanTimeoutMs) {
            ESP_LOGW(kTag, "Wi-Fi scan unavailable after %lu ms",
                     static_cast<unsigned long>(scan_elapsed));
            wifi_scan_pending_ = false;
            store_.set_wifi_networks({}, WifiScanStatus::Failed);
        } else {
            wifi_scan_retry_due_ms_ = now_ms + kWifiScanRetryDelayMs;
        }
    }

    if (wifi_scan_in_progress_) {
        const uint32_t scan_elapsed = now_ms - wifi_scan_started_ms_;
        if (scan_elapsed >= kWifiScanTimeoutMs) {
            ESP_LOGW(kTag, "Wi-Fi scan timed out after %lu ms",
                     static_cast<unsigned long>(scan_elapsed));
            wifi_scan_in_progress_ = false;
            store_.set_wifi_networks({}, WifiScanStatus::Failed);
        }
    }

    if (!wifi_timeout_armed_) {
        return;
    }

    const uint32_t elapsed = now_ms - wifi_connect_started_ms_;
    if (elapsed < kWifiConnectTimeoutMs) {
        return;
    }

    ESP_LOGW(kTag, "Wi-Fi connect timed out after %lu ms",
             static_cast<unsigned long>(elapsed));
    wifi_timeout_armed_ = false;
    schedule_wifi_reconnect(now_ms);
}

void SetupService::handle_submission(const OnboardingSubmission& submission)
{
    nvs_handle_t handle{};
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK) {
        ESP_LOGE(kTag, "nvs_open (write) failed");
        return;
    }

    switch (submission.step) {
        case OnboardingStep::DisplayName: {
            prefs_.display_name = submission.display_name;
            nvs_set_str(handle, kKeyDisplayName, prefs_.display_name.c_str());
            nvs_commit(handle);
            store_.set_preferences(prefs_);
            store_.set_onboarding_step(OnboardingStep::Wifi);
            break;
        }
        case OnboardingStep::Wifi: {
            if (submission.wifi_ssid.empty()) {
                wifi_auto_reconnect_enabled_ = false;
                wifi_connect_pending_ = false;
                wifi_timeout_armed_ = false;
                wifi_retry_count_ = 0;
                esp_wifi_disconnect();
                nvs_erase_key(handle, kKeyWifiSsid);
                nvs_erase_key(handle, kKeyWifiPassword);
                nvs_commit(handle);
                nvs_close(handle);
                store_.set_wifi_status(WifiConnectStatus::Idle);
                return;
            }

            nvs_set_str(handle, kKeyWifiSsid, submission.wifi_ssid.c_str());
            nvs_set_str(handle, kKeyWifiPassword, submission.wifi_password.c_str());
            nvs_commit(handle);

            wifi_config_t wifi_config{};
            std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid),
                         submission.wifi_ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
            std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password),
                         submission.wifi_password.c_str(), sizeof(wifi_config.sta.password) - 1);

            wifi_auto_reconnect_enabled_ = true;
            wifi_retry_count_ = 0;
            store_.set_wifi_status(WifiConnectStatus::Connecting);
            if (esp_wifi_set_config(WIFI_IF_STA, &wifi_config) != ESP_OK) {
                ESP_LOGW(kTag, "esp_wifi_set_config failed");
                wifi_auto_reconnect_enabled_ = false;
                store_.set_wifi_status(WifiConnectStatus::Failed);
            } else {
                begin_wifi_connect();
            }
            break;
        }
        case OnboardingStep::TimezoneAndFormat: {
            prefs_.timezone = submission.timezone;
            prefs_.time_format_24h = submission.time_format_24h;
            nvs_set_str(handle, kKeyTimezone, prefs_.timezone.c_str());
            nvs_set_u8(handle, kKeyTimeFormat24h, prefs_.time_format_24h ? 1 : 0);
            nvs_commit(handle);
            store_.set_preferences(prefs_);
            store_.set_onboarding_step(OnboardingStep::Confirmation);
            break;
        }
        case OnboardingStep::Confirmation: {
            nvs_set_u8(handle, kKeyOnboardingDone, 1);
            nvs_commit(handle);
            store_.set_onboarding_step(OnboardingStep::Done);
            store_.set_onboarding_needed(false);
            store_.set_screen(ScreenId::Home);
            break;
        }
        case OnboardingStep::Done:
            break;
    }

    nvs_close(handle);
}

void SetupService::handle_wifi_scan_request()
{
    wifi_scan_pending_ = false;
    wifi_scan_started_ms_ = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    if (try_wifi_scan()) {
        return;
    }

    ESP_LOGW(kTag, "Wi-Fi scan deferred until ESP-Hosted link is ready");
    wifi_scan_pending_ = true;
    wifi_scan_retry_due_ms_ = wifi_scan_started_ms_ + kWifiScanRetryDelayMs;
}

bool SetupService::try_wifi_scan()
{
    if (wifi_scan_in_progress_) {
        return true;
    }

    const esp_err_t scan_err = esp_wifi_scan_start(nullptr, false);
    if (scan_err != ESP_OK) {
        ESP_LOGW(kTag, "esp_wifi_scan_start failed: %s", esp_err_to_name(scan_err));
        return false;
    }

    wifi_scan_pending_ = false;
    wifi_scan_in_progress_ = true;
    ESP_LOGI(kTag, "wifi scan started");
    return true;
}

void SetupService::complete_wifi_scan()
{
    uint16_t found = 0;
    esp_wifi_scan_get_ap_num(&found);
    found = std::min(found, kMaxScanResults);

    std::vector<wifi_ap_record_t> records(found);
    if (found > 0 &&
        esp_wifi_scan_get_ap_records(&found, records.data()) != ESP_OK) {
        ESP_LOGW(kTag, "esp_wifi_scan_get_ap_records failed");
        wifi_scan_in_progress_ = false;
        store_.set_wifi_networks({}, WifiScanStatus::Failed);
        return;
    }

    std::vector<WifiNetwork> networks;
    networks.reserve(found);
    for (uint16_t i = 0; i < found; ++i) {
        const auto& rec = records[i];
        const char* ssid = reinterpret_cast<const char*>(rec.ssid);
        if (ssid[0] == '\0') {
            continue;
        }

        auto it = std::find_if(networks.begin(), networks.end(),
            [&](const WifiNetwork& network) { return network.ssid == ssid; });
        if (it != networks.end()) {
            if (rec.rssi > it->rssi) {
                it->rssi = rec.rssi;
            }
            continue;
        }

        networks.push_back(WifiNetwork{
            ssid,
            static_cast<int8_t>(rec.rssi),
            rec.authmode != WIFI_AUTH_OPEN,
        });
    }

    std::sort(networks.begin(), networks.end(),
        [](const WifiNetwork& a, const WifiNetwork& b) {
            return a.rssi > b.rssi;
        });

    ESP_LOGI(kTag, "wifi scan: %u network(s) after de-dup",
             static_cast<unsigned>(networks.size()));
    wifi_scan_in_progress_ = false;
    store_.set_wifi_networks(networks, WifiScanStatus::Done);
}

void SetupService::on_wifi_scan_done()
{
    complete_wifi_scan();
}

void SetupService::on_wifi_connected()
{
    ESP_LOGI(kTag, "Wi-Fi connected (got IP)");
    wifi_retry_count_ = 0;
    wifi_connect_pending_ = false;
    wifi_timeout_armed_ = false;
    store_.set_wifi_status(WifiConnectStatus::Connected);
    if (store_.snapshot().onboarding.step == OnboardingStep::Wifi) {
        store_.set_onboarding_step(OnboardingStep::TimezoneAndFormat);
    }
    esp_netif_sntp_start();
    ESP_LOGI(kTag, "SNTP started after Wi-Fi got IP");
}

void SetupService::on_wifi_disconnected()
{
    if (!wifi_auto_reconnect_enabled_) {
        return;
    }

    const WifiConnectStatus current = store_.snapshot().onboarding.wifi_status;
    if (current == WifiConnectStatus::Connected ||
        current == WifiConnectStatus::Connecting) {
        const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
        schedule_wifi_reconnect(now_ms);
    }
}

void SetupService::begin_wifi_connect()
{
    wifi_connect_pending_ = false;
    wifi_timeout_armed_ = true;
    wifi_connect_started_ms_ = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

    if (esp_wifi_connect() != ESP_OK) {
        ESP_LOGW(kTag, "esp_wifi_connect failed to start");
        wifi_timeout_armed_ = false;
        schedule_wifi_reconnect(wifi_connect_started_ms_);
    }
}

void SetupService::schedule_wifi_reconnect(uint32_t now_ms)
{
    if (wifi_retry_count_ >= kMaxWifiRetries) {
        ESP_LOGW(kTag, "Wi-Fi reconnect exhausted after %d retries", kMaxWifiRetries);
        wifi_connect_pending_ = false;
        wifi_timeout_armed_ = false;
        store_.set_wifi_status(WifiConnectStatus::Failed);
        return;
    }

    ++wifi_retry_count_;
    wifi_connect_pending_ = true;
    wifi_timeout_armed_ = false;
    wifi_reconnect_due_ms_ = now_ms + kWifiReconnectDelayMs;
    store_.set_wifi_status(WifiConnectStatus::Connecting);
    ESP_LOGW(kTag, "Wi-Fi reconnect scheduled %d/%d",
             wifi_retry_count_, kMaxWifiRetries);
}

}  // namespace nova
