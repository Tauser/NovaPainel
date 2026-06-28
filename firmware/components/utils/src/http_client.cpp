#include "http_client.hpp"

#if defined(ESP_PLATFORM)

#include <mutex>
#include <utility>

#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"

namespace nova {

namespace {

// Serializa TODAS as requisicoes HTTP do firmware. Como cada provider passa
// por aqui, este mutex garante no maximo 1 handshake TLS por vez. Sem ele,
// Market+Forex+Weather abrem 3 TLS simultaneos no boot (~130 KB de RAM cada),
// espremendo a RAM interna -> falhas -> circuit breaker abre -> "nao carrega".
// Tambem reduz a contencao de barramento que causa o flash do DSI. A
// serializacao por prioridade/escalonamento fica no NetworkWorker (ver ADR).
std::mutex& http_gate()
{
    static std::mutex gate;
    return gate;
}

struct HttpBody {
    std::string data;
};

esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
    auto* body = static_cast<HttpBody*>(evt->user_data);
    if (!body) {
        return ESP_OK;
    }

    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        body->data.append(static_cast<const char*>(evt->data), evt->data_len);
    }
    return ESP_OK;
}

}  // namespace

bool http_get(const char* tag, const char* url, std::string& body,
              int timeout_ms, int buffer_size)
{
    // 1 requisicao HTTP por vez em todo o firmware (ver http_gate()).
    std::lock_guard<std::mutex> serialize(http_gate());

    HttpBody payload;
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
        return false;
    }

    const esp_err_t err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200 || payload.data.empty()) {
        ESP_LOGW(tag, "HTTP failed: err=%s status=%d", esp_err_to_name(err), status);
        return false;
    }

    body = std::move(payload.data);
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
