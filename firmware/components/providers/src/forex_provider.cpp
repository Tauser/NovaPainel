// NovaPainel - providers/forex_provider.cpp
// Hardware-only: esp_http_client + mbedtls cert bundle + cJSON, none of
// which have a host shim (see tools/scripts/host_check.sh SKIP_FILES). Same
// NTP/TLS prerequisite as CoinGeckoProvider/OpenMeteoProvider (ADR-0021).
#include "forex_provider.hpp"

#include <cstdlib>
#include <cstring>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"

namespace nova {

namespace {
constexpr const char* kTag = "ForexProvider";

constexpr const char* kUrl = "https://economia.awesomeapi.com.br/json/last/USD-BRL";

constexpr int kMaxResponseBytes = 512;

struct FetchContext {
    char buf[kMaxResponseBytes];
    int  len{0};
};

esp_err_t http_event_handler(esp_http_client_event_t* evt) {
    if (evt->event_id != HTTP_EVENT_ON_DATA) return ESP_OK;
    auto* ctx = static_cast<FetchContext*>(evt->user_data);
    if (ctx->len + evt->data_len < kMaxResponseBytes) {
        std::memcpy(ctx->buf + ctx->len, evt->data, evt->data_len);
        ctx->len += evt->data_len;
    }
    return ESP_OK;
}
}  // namespace

Result<double> ForexProvider::fetch_usd_brl() {
    FetchContext ctx{};

    esp_http_client_config_t config = {};
    config.url = kUrl;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.event_handler = &http_event_handler;
    config.user_data = &ctx;
    config.timeout_ms = 8000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    const esp_err_t err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200) {
        ESP_LOGW(kTag, "http failed: err=%s status=%d", esp_err_to_name(err), status);
        return Result<double>::fail(ErrorCode::Network, "awesomeapi http failed");
    }

    ctx.buf[ctx.len < kMaxResponseBytes ? ctx.len : kMaxResponseBytes - 1] = '\0';

    cJSON* root = cJSON_Parse(ctx.buf);
    if (!root) {
        ESP_LOGW(kTag, "json parse failed: '%s'", ctx.buf);
        return Result<double>::fail(ErrorCode::Parse, "awesomeapi json parse failed");
    }

    // Response shape: {"USDBRL": {"bid": "5.4250", ...}} - "bid" (and every
    // other numeric-looking field) comes as a JSON *string*, not a number.
    cJSON* pair = cJSON_GetObjectItemCaseSensitive(root, "USDBRL");
    cJSON* bid = pair ? cJSON_GetObjectItemCaseSensitive(pair, "bid") : nullptr;

    if (!cJSON_IsString(bid) || bid->valuestring == nullptr) {
        ESP_LOGW(kTag, "json missing USDBRL.bid field");
        cJSON_Delete(root);
        return Result<double>::fail(ErrorCode::Parse, "awesomeapi json missing fields");
    }

    const double rate = std::atof(bid->valuestring);
    cJSON_Delete(root);

    if (rate <= 0.0) {
        return Result<double>::fail(ErrorCode::Parse, "awesomeapi invalid rate");
    }
    return Result<double>::ok(rate);
}

}  // namespace nova
