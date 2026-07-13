#pragma once

#include "status.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace nova {

// Persistencia offline-first em LittleFS (RESOURCE-BUDGET.md secao 4): o
// cache existe só para sobreviver a um boot sem rede -- o StateStore em RAM
// continua sendo quem serve a UI. Blobs sao bytes opacos para manter cache/
// desacoplado de models/ (quem serializa/desserializa e o chamador em
// services/).
class CacheStore {
public:
    Status mount();
    bool ready() const { return ready_; }

    // True se uma escrita para `domain` agora seria descartada pelo
    // throttle. Puro (não toca flash nem estado de sessão), exposto para
    // teste host e para quem quiser evitar montar o payload à toa.
    bool would_throttle(const char* domain, uint64_t now_ms) const;

    // Escreve o blob de `domain` (nome curto, ex. "market"), atômico via
    // tmp+rename. No máximo 1 escrita a cada 30 min por domain nesta sessão
    // (RESOURCE-BUDGET.md: erase de flash compete com o refresh do
    // display) -- chamada dentro da janela retorna RateLimited sem tocar a
    // flash. `now_ms` deve ser monotônico (esp_timer_get_time()/1000).
    Status save(const char* domain, const void* data, std::size_t size, uint64_t now_ms);

    // Lê o blob de `domain`. Falha com Unavailable se nunca foi escrito, se
    // o mount falhou, ou se o header não bate com `expected_size` (mesma
    // regra do SetupStorageLogic: schema incompatível/corrompido é
    // ignorado, nunca interpretado às cegas).
    Result<std::vector<uint8_t>> load(const char* domain, std::size_t expected_size) const;

private:
    struct DomainThrottle {
        char domain[24]{};
        uint64_t last_write_ms{0};
        bool used{false};
        bool written_this_session{false};
    };

    static constexpr std::size_t kMaxDomains = 8;
    static constexpr uint32_t kMinWriteIntervalMs = 30 * 60 * 1000;

    const DomainThrottle* find_throttle(const char* domain) const;
    DomainThrottle* find_or_add_throttle(const char* domain);

    std::array<DomainThrottle, kMaxDomains> throttle_{};
    bool ready_{false};

#if !defined(ESP_PLATFORM)
    struct HostEntry {
        char domain[24]{};
        std::vector<uint8_t> bytes{};
        bool present{false};
    };
    std::array<HostEntry, kMaxDomains> host_entries_{};
#endif
};

}  // namespace nova
