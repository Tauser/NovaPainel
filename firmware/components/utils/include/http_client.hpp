#pragma once

#include "status.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace nova {

// Open-Meteo ~15 KB, CoinGecko ~5 KB, AwesomeAPI <1 KB -- 48 KB cobre com
// folga (RESOURCE-BUDGET.md §2 regra 2). Resposta maior que o cap é FALHA do
// request (Truncated), nunca truncamento silencioso.
constexpr std::size_t kHttpBodyCapBytes = 48 * 1024;
constexpr uint32_t kHttpDefaultTimeoutMs = 8000;

struct HttpResponse {
    int status_code{0};
    std::string body{};
};

// Um GET HTTPS por vez em todo o firmware, via mutex interno
// (RESOURCE-BUDGET.md §1 regra 1: 3 handshakes TLS simultâneos no v3
// esgotavam a SRAM interna e derrubavam serviços em cascata). NetworkWorker
// já serializa no nível de política (RequestOrchestrator), mas o mutex aqui
// é a garantia estrutural -- vale mesmo se algo chamar HttpClient::get()
// fora do NetworkWorker.
class HttpClient {
public:
    Result<HttpResponse> get(const std::string& url, uint32_t timeout_ms = kHttpDefaultTimeoutMs);
};

}  // namespace nova
