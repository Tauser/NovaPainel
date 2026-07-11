# NovaPanel — Orçamento de Recursos (baseline v4)

> **Contrato físico da plataforma.** Consolida em um lugar o conhecimento que
> no v3 vivia espalhado em comentários — pago com semanas de depuração de
> glitches reais. Violar este documento é regressão, não opção de design.
> Números marcados [v3] foram medidos em bancada no baseline anterior.

## 1. O recurso escasso desta placa

O ESP32-P4 compartilha o barramento MSPI entre **flash** e **PSRAM**, e o
controlador MIPI-DSI lê o framebuffer da PSRAM continuamente (1024×600@60 Hz).
Consequência: **qualquer consumidor agressivo de PSRAM ou qualquer erase de
flash compete com o refresh do display.** Os sintomas observados [v3]:

- flash branco / underrun do DSI durante download+parse de HTTP em PSRAM;
- glitch visual a cada `nvs_commit`/escrita LittleFS (erase de flash);
- SRAM interna caindo a ~173 KB com 3 handshakes TLS simultâneos
  (~130 KB cada) → falhas em cascata → circuit breakers abrindo.

## 2. Mapa de memória e regras de alocação

| Recurso | Uso reservado | Regra |
|---|---|---|
| PSRAM (32 MB) | Framebuffers do display (double buffer full-size), assets grandes de UI (imagens de álbum futuro) | Nenhum dado tocado em burst durante render (HTTP, JSON, TLS) pode viver aqui |
| SRAM interna | TLS, corpo HTTP, nós cJSON, buffers de áudio, stacks | Tudo que é parsing/rede aloca com `MALLOC_CAP_INTERNAL` |
| Flash (app) | ver tabela de partições (OPERATIONS.md) | Escrita de flash em runtime é throttlada (§4) |

Regras derivadas (obrigatórias):

1. **1 conexão HTTPS por vez** em todo o firmware (mutex no http_client) +
   `CONFIG_MBEDTLS_DYNAMIC_BUFFER=y`. [v3, ADR-0004 v4]
2. Corpo HTTP: buffer em SRAM interna, cap **48 KB** (Open-Meteo ~15 KB,
   CoinGecko OHLC ~30 KB). Resposta maior que o cap = request FALHOU
   (breaker conta), nunca truncamento silencioso. Aumentar o cap exige
   atualizar esta tabela e medir heap.
3. cJSON com hooks para SRAM interna (`cJSON_InitHooks`). [v3]
4. Render LVGL em modo **parcial** (não FULL): só região suja, poupa banda
   de PSRAM para o DSI. [v3]
5. Rotação 180° por PPA (`sw_rotate`); o EK79007 não gira por MADCTL;
   `avoid_tearing` é incompatível com sw_rotate neste BSP. [v3]
6. Stack da lvgl_task: 16 KB [v3]. Stack do net_worker: 8 K words [v3].

## 3. Orçamento de RAM interna (limiares operacionais)

| Situação | Limiar | Ação |
|---|---|---|
| Watermark de heap interno em operação normal | > 80 KB livres | — |
| Watermark abaixo de 60 KB | `ResourceWarning` | log + métrica + desliga modo focado/candles |
| Watermark abaixo de 40 KB | crítico | suspende fetchers não-Critical até recuperar |

O SystemService amostra watermarks a cada tick lento e publica os valores no
estado (visíveis na tela de sistema). Sem medição não há orçamento.

## 4. Orçamento de escrita de flash em runtime

- Cache LittleFS: persistir no máximo **1×/30 min por domínio** (o StateStore
  serve a UI em RAM; o cache existe para o boot offline). [v3]
- NVS: dedup obrigatório — não regravar valor idêntico; agrupar commits.
- Nenhuma escrita de flash iniciada por callback de toque.
- OTA (futuro): download+write de imagem é o pior caso do barramento —
  executar com UI em modo reduzido (tela de update), nunca durante uso normal.

## 5. Orçamento de rede (políticas default do orquestrador)

| Domínio | Intervalo | Rate | Prioridade |
|---|---|---|---|
| Clima (Open-Meteo) | 30 min | 6/min | Normal |
| BTC spot (CoinGecko) | 3 min | 6/min | Normal |
| USD/BRL (AwesomeAPI) | 60 min | 6/min | Normal |
| OHLC/candles | 5 min, só com tela Market ativa (modo focado) | 6/min | Low |

Gap mínimo entre buscas consecutivas: 400 ms [v3]. Boot: fetchers entram
escalonados, nunca simultâneos.

## 6. Barramentos e periféricos compartilhados

- **I2C único**: GT911 (touch) + ES8311 (codec). O polling de touch roda
  dentro do display lock ⇒ acesso a registrador do codec exige o mesmo lock.
  Encapsulado EXCLUSIVAMENTE em `board::lock_shared_i2c()` — proibido
  adquirir display lock "para mexer no codec" fora do board/. [v3]
- I2S (áudio) independe do I2C — escrita de amostras não precisa de lock. [v3]
- SDIO P4↔C6 (ESP-Hosted): tráfego Wi-Fi compete com o mesmo mundo físico;
  rajadas de rede durante animações pesadas são detectáveis — medir antes de
  adicionar domínio de rede novo.

## 7. Gotchas fatais conhecidos (proibições)

- `static std::function` global → overflow de `__cxa_atexit` → freeze no boot.
- `bsp_audio_init(nullptr)` → crash + coredump corrompido + boot loop.
- Múltiplos TLS simultâneos (ver §1).
- `nvs_commit` em rajada durante render (ver §1).

## 8. Processo

Feature nova com custo de RAM/rede/flash relevante: adicionar linha na tabela
correspondente ANTES da implementação (PR de doc), com estimativa; substituir
por número medido na entrega. Divergência estimado→medido > 50% = revisar a
decisão em ADR.
