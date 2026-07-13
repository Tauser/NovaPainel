#include "clock_service.hpp"

#include "esp_log.h"

#if defined(ESP_PLATFORM)
#include "esp_err.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#endif

namespace nova {
namespace {
constexpr uint64_t kPlausibleUnixTimeThreshold = 1704067200ULL;  // 2024-01-01T00:00:00Z
constexpr const char* kTag = "ClockService";

bool is_plausible_time(uint64_t unix_time_s) {
    return unix_time_s >= kPlausibleUnixTimeThreshold;
}

// Monotonic ms-since-boot, matching the contract every other DataStatus
// producer uses for last_update_ms (e.g. SetupService::current_time_ms()) --
// never wall-clock epoch time, which would make staleness math wrap/overflow.
uint64_t monotonic_now_ms() {
#if defined(ESP_PLATFORM)
    return static_cast<uint64_t>(esp_timer_get_time() / 1000);
#else
    return 0;
#endif
}

}  // namespace

ClockService::ClockService(StateStore& state_store, const IBoard& board, DataSource source_when_valid)
    : state_store_(state_store), board_(board), source_when_valid_(source_when_valid) {}

void ClockService::maybe_start_sntp() {
#if defined(ESP_PLATFORM)
    if (sntp_synced_ || sntp_start_failed_) {
        return;
    }

    const SetupState setup = state_store_.setup();
    if (setup.wifi_connect_status != WifiConnectStatus::Connected) {
        return;
    }

    if (!sntp_initialized_) {
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        config.start = false;
        config.wait_for_sync = true;
        const esp_err_t init_err = esp_netif_sntp_init(&config);
        if (init_err != ESP_OK && init_err != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(kTag, "SNTP init failed: %s", esp_err_to_name(init_err));
            sntp_start_failed_ = true;
            return;
        }
        sntp_initialized_ = true;
    }

    if (!sntp_started_) {
        const esp_err_t start_err = esp_netif_sntp_start();
        if (start_err != ESP_OK) {
            ESP_LOGW(kTag, "SNTP start failed: %s", esp_err_to_name(start_err));
            sntp_start_failed_ = true;
            return;
        }
        sntp_started_ = true;
        ESP_LOGI(kTag, "SNTP started after Wi-Fi got IP");
    }
#endif
}

void ClockService::poll_sntp_sync() {
#if defined(ESP_PLATFORM)
    if (!sntp_started_ || sntp_synced_) {
        return;
    }

    const esp_err_t sync_err = esp_netif_sntp_sync_wait(0);
    if (sync_err == ESP_OK || is_plausible_time(board_.rtc_unix_time_s())) {
        sntp_synced_ = true;
        ESP_LOGI(kTag, "SNTP time synchronized");
    } else if (sync_err != ESP_ERR_TIMEOUT && sync_err != ESP_ERR_NOT_FINISHED) {
        ESP_LOGW(kTag, "SNTP sync wait failed: %s", esp_err_to_name(sync_err));
    }
#endif
}

void ClockService::tick() {
    maybe_start_sntp();
    poll_sntp_sync();

    const uint64_t rtc_unix_time_s = board_.rtc_unix_time_s();
    ClockState clock = state_store_.clock();

    if (!is_plausible_time(rtc_unix_time_s)) {
        if (!clock.valid || clock.source != DataSource::None || clock.unix_time_s != 0) {
            clock.valid = false;
            clock.stale = true;
            clock.source = DataSource::None;
            clock.last_update_ms = 0;
            clock.unix_time_s = 0;
            state_store_.set_clock(clock);
        }
        if (!has_reported_state_ || last_plausible_state_) {
            ESP_LOGI(kTag, "no plausible RTC time yet");
        }
        has_reported_state_ = true;
        last_plausible_state_ = false;
        return;
    }

    if (clock.valid && last_published_unix_time_s_ == rtc_unix_time_s) {
        return;
    }

    clock.valid = true;
    clock.stale = false;
    clock.source = source_when_valid_;
    clock.last_update_ms = monotonic_now_ms();
    clock.unix_time_s = rtc_unix_time_s;
    state_store_.set_clock(clock);
    if (!has_reported_state_ || !last_plausible_state_) {
        ESP_LOGI(kTag, "plausible RTC time detected: %llu",
                 static_cast<unsigned long long>(rtc_unix_time_s));
    }
    has_reported_state_ = true;
    last_published_unix_time_s_ = rtc_unix_time_s;
    last_plausible_state_ = true;
}

void ClockService::tick_adapter(void* context) {
    auto* service = static_cast<ClockService*>(context);
    if (service != nullptr) {
        service->tick();
    }
}

}  // namespace nova
