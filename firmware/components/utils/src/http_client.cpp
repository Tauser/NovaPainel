#include "http_client.hpp"

#if defined(ESP_PLATFORM)

#include <cstring>
#include <mutex>
#include <utility>

#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"

namespace nova {

namespace {

// Tamanho máximo do body de resposta alocado na SRAM interna.
// OpenMeteo ~15 KB, CoinGecko OHLC ~30 KB → 48 KB cobre com folga.
// Alocado em SRAM interna (não PSRAM) para evitar contenção de barramento
// com o DMA do display durante o download/parsing — causa do DSI underrun
// (flash azul) que aparecia a cada atualização de serviço.
constexpr size_t kBodyCap = 48 * 1024;

// Serializa TODAS as requisicoes HTTP do firmware. Como cada provider passa
// por aqui, este mutex garante no maximo 1 handshake TLS por vez. Sem ele,
// Market+Forex+Weather abrem 3 TLS simultaneos no boot (~130 KB de RAM cada),
// espremendo a RAM interna -> falhas -> circuit breaker abre -> "nao carrega".
std::mutex& http_gate()
{
    static std::mutex gate;
    return gate;
}

struct HttpBody {
    char*  buf;   // apontador para buffer em SRAM interna (heap_caps_malloc)
    size_t len;   // bytes acumulados
    size_t cap;   // capacidade do buffer
};

esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
    auto* body = static_cast<HttpBody*>(evt->user_data);
    if (!body || evt->event_id != HTTP_EVENT_ON_DATA) return ESP_OK;

    const size_t available = body->cap - body->len;
    const size_t to_copy   = (evt->data_len <= available) ? evt->data_len : available;
    if (to_copy > 0) {
        std::memcpy(body->buf + body->len, evt->data, to_copy);
        body->len += to_copy;
    }
    if (to_copy < static_cast<size_t>(evt->data_len)) {
        ESP_LOGW("http", "response truncated at %zu B (cap=%zu)", body->len, body->cap);
    }
    return ESP_OK;
}

}  // namespace

bool http_get(const char* tag, const char* url, std::string& body,
              int timeout_ms, int buffer_size)
{
    // 1 requisicao HTTP por vez em todo o firmware (ver http_gate()).
    std::lock_guard<std::mutex> serialize(http_gate());

    // Aloca o buffer de recepção na SRAM interna.
    // PSRAM fica com via livre exclusiva para o DMA do display (framebuffer).
    char* raw = static_cast<char*>(
        heap_caps_malloc(kBodyCap, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!raw) {
        ESP_LOGE(tag, "OOM: %zu B SRAM interna para HTTP body indisponivel", kBodyCap);
        return false;
    }

    HttpBody payload{raw, 0, kBodyCap};

    esp_http_client_config_t config{};
    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = timeout_ms;
    config.buffer_size = buffer_size;
    config.event_handler = http_event_handler;
    config.user_data = &payload;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGW(tag, "esp_http_client_init failed");
        heap_caps_free(raw);
        return false;
    }

    const esp_err_t err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200 || payload.len == 0) {
        ESP_LOGW(tag, "HTTP failed: err=%s status=%d", esp_err_to_name(err), status);
        heap_caps_free(raw);
        return false;
    }

    body.assign(raw, payload.len);
    heap_caps_free(raw);
    return true;
}

}  // namespace nova

#else

namespace nova {

bool http_get(const char*, const char*, std::string&, int, int)
{
    return false;
}

}  // namespace nova

#endif
