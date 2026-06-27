// NovaPainel - providers/coingecko_provider.cpp
// Hardware-only: esp_http_client + mbedtls cert bundle + cJSON, none of
// which have a host shim (see tools/scripts/host_check.sh SKIP_FILES).
// Needs the system clock to be NTP-synced (ADR-0021) for TLS cert
// validation to succeed - SetupService starts SNTP once Wi-Fi connects.
#include "coingecko_provider.hpp"

#include <cstring>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"

namespace nova {

namespace {
constexpr const char* kTag = "CoinGeckoProvider";

// BTC in USD (price + 24h change). USD/BRL used to be triangulated from a
// bitcoin/brl quote in this same response - now comes from a dedicated
// ForexProvider/AwesomeAPI instead (ROADMAP Fase 5 follow-up), so this only
// asks CoinGecko for what it alone provides.
constexpr const char* kUrl =
    "https://api.coingecko.com/api/v3/simple/price"
    "?ids=bitcoin&vs_currencies=usd&include_24hr_change=true";

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

Result<MarketSummary> CoinGeckoProvider::fetch_summary() {
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
        return Result<MarketSummary>::fail(ErrorCode::Network, "coingecko http failed");
    }

    ctx.buf[ctx.len < kMaxResponseBytes ? ctx.len : kMaxResponseBytes - 1] = '\0';

    cJSON* root = cJSON_Parse(ctx.buf);
    if (!root) {
        ESP_LOGW(kTag, "json parse failed: '%s'", ctx.buf);
        return Result<MarketSummary>::fail(ErrorCode::Parse, "coingecko json parse failed");
    }

    cJSON* bitcoin = cJSON_GetObjectItemCaseSensitive(root, "bitcoin");
    cJSON* usd = bitcoin ? cJSON_GetObjectItemCaseSensitive(bitcoin, "usd") : nullptr;
    cJSON* change = bitcoin ? cJSON_GetObjectItemCaseSensitive(bitcoin, "usd_24h_change") : nullptr;

    if (!cJSON_IsNumber(usd)) {
        ESP_LOGW(kTag, "json missing usd field");
        cJSON_Delete(root);
        return Result<MarketSummary>::fail(ErrorCode::Parse, "coingecko json missing fields");
    }

    MarketSummary s{};
    s.btc_usd = usd->valuedouble;
    s.btc_change_24h = cJSON_IsNumber(change) ? change->valuedouble : 0.0;
    s.valid = true;
    s.stale = false;
    s.source = DataSource::Live;

    cJSON_Delete(root);
    return Result<MarketSummary>::ok(s);
}

}  // namespace nova
