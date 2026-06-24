# NovaPainel - Roadmap

> Estado atual: **Fase 3 concluída** - `WaveshareBoard` real (display EK79007
> + touch GT911 via BSP oficial, SD card real, link SDIO ao C6) substituiu
> `MockBoard` em `firmware/main/app_main.cpp`, validado na placa física
> (ADR-0016). LVGL, Wi-Fi/APIs reais ainda não implementados, por decisão
> (Fases 4/5).

## Princípio de ordenação: risco antes de hardening

A regra é **provar a fundação antes de endurecer**. Não se otimiza render,
PSRAM ou resiliência de rede antes do BSP/LVGL e do link P4↔C6 existirem e serem
validados. Hardening de produção (segurança, performance, soak test) vem **depois**
do hardware provado, não antes. Por isso a Fase 0 (viabilidade de hardware) é
pré-requisito de tudo.

```text
Fase 0  - Risk Gates de hardware (C6/ESP-Hosted, SDIO, Wi-Fi+display simultâneos,
          HTTPS, NTP, WebSocket, SD+Wi-Fi, PSRAM, display/touch, RTC, partições,
          brownout/térmica)                                        [bloqueia tudo]
Fase 1  - Organização do monorepo                                  [feito]
Fase 2  - Firmware core com mocks                                  [feito]
Fase 3  - Board real / BSP                                          [feito]
Fase 4  - Display/touch/LVGL real (buffers PSRAM pré-alocados, dirty rect nativo)
Fase 5  - Wi-Fi/NTP/HTTPS/CoinGecko snapshot
Fase 6  - Home MVP com cache (LittleFS, escrita atômica, dado stale visível)
Fase 7  - Sistema/status/logs (observabilidade: coredump, motivo de reset)
Fase 8  - Resiliência de produção (circuit breaker, backpressure, degradação)
Fase 9  - Hardening de segurança (NVS Encryption, Secure Boot v2, Flash Encryption)
Fase 10 - Performance sob carga (render parcial, candles incrementais, soak 72h)
Fase 11 - Modularização de serviços (multi-provider, NotificationService, RTC híbrido)
Fase 12 - v1.0: modo noite, álbum, timer, perfis simples
Fase 13 - sensores: LD2410C, BH1750, BME280
Fase 14 - futuro: Sonoff/automação física
Fase 15 - futuro: server opcional/NoiseBot
```

## Detalhe das fases iniciais

- **Fase 0 (bloqueante):** tratar rede e display como risco de hardware. Provar
  boot estável, link P4↔C6 (SDIO/ESP-Hosted), **Wi-Fi + display + touch em uso
  simultâneo por horas sem reset** (risco confirmado em campo em 2026), HTTPS,
  NTP, PSRAM, framebuffer em PSRAM, SD, RTC e particionamento. Sem passar a Fase 0,
  o produto não evolui. Checklist preenchível em `docs/FASE0-CHECKLIST.md`. Ver
  `HARDWARE.md` e `skills/novapanel-hardware-risk-gate`.
  - **Fase 0.1 — harness de coexistência (Gate 15) [entregável]:** construir o
    firmware de teste em `firmware/experiments/gate15_coexistence/` que roda
    display (framebuffer em PSRAM) + touch + Wi-Fi/HTTPS sustentado ao mesmo tempo,
    com instrumentação (uptime, motivo de reset, contador de reboots, watermark de
    heap/PSRAM, reassociações Wi-Fi). É o experimento que valida — ou redireciona —
    toda a Fase 0. Critérios PASS/FAIL no Gate 15 do `FASE0-CHECKLIST.md`.
- **Fase 2 (concluída):** core (`EventBus`, `StateStore`, `Service`/`ServiceManager`,
  `RequestOrchestrator`, `UiDispatcher`), serviços mock (`ClockService`,
  `MarketService`, `NotificationService`), `MockBoard`, `MockMarketProvider`,
  `HomeScreen` por logs. Objetivo: validar o fluxo de dados ponta a ponta.
- **Fase 3 (concluída):** `MockBoard` substituído pelo BSP Waveshare.
  `WaveshareBoard` real (display EK79007 + touch GT911 via
  `waveshare/esp32_p4_wifi6_touch_lcd_7b`, SD card via SDMMC + LDO canal 4,
  link ESP-Hosted/SDIO ao C6 sem associar a um AP - ADR-0016) roda em
  `firmware/main/app_main.cpp` e foi validado na placa física
  (`board=1 display=1 touch=1 net=1 sd=1` em `SystemStatus`). `cache_ready`
  fica false até a Fase 6 (camada LittleFS é separada do mount cru do SD).
- **Fase 4:** integrar LVGL real; `UiDispatcher::process_pending()` passa a rodar
  na `lvgl_task` (fila assíncrona com coalescing, ADR-0013); `HomeScreen::render`
  passa de logs para widgets. Framebuffers pré-alocados em PSRAM no boot; dirty
  rectangles via API nativa do LVGL. Pinagem de cores definida e documentada.
  Tela de Boot/splash entra aqui também (`ScreenId::Boot` já existe em
  `AppState`, só falta o screen builder - ADR-0017).
- **Fase 5:** trocar `MockMarketProvider` por `CoinGeckoProvider` (REST, 60s,
  6 req/min, cache, batch). Adicionar provider de clima (ex.: Open-Meteo) e fonte
  USD/BRL dedicada (atrás da mesma `IMarketProvider`/interface de câmbio). Tela
  de setup/provisionamento de Wi-Fi entra aqui: UI publica intenção via
  `EventBus`, um `SetupService` é o único a chamar `esp_wifi_connect`/persistir
  credencial em NVS (ADR-0017 + ADR-0011) - UI nunca chama Wi-Fi direto.

## Detalhe das fases de hardening (pós-hardware provado)

- **Fase 6:** cache em LittleFS com escrita atômica e versionamento de schema
  (ADR-0015); UI sinaliza dado stale / sem-rede / sem-NTP.
- **Fase 7:** tela de sistema com observabilidade real — partição de coredump,
  motivo do último reset, contador de reboots, idade do dado por domínio (ADR-0014).
- **Fase 8:** resiliência no `RequestOrchestrator` — circuit breaker por domínio
  (closed/open/half-open), backoff exponencial + jitter, backpressure com
  drop-oldest, retry finito, degradação para cache (ADR-0012).
- **Fase 9:** segurança de produção — NVS Encryption nativa, Secure Boot v2 com
  anti-rollback, Flash Encryption; chaves/eFuses e processo de release (ADR-0011).
  Dev pode manter desativado; o **layout de partição** já assume isso desde a Fase 3.
- **Fase 10:** performance sob carga — render parcial por dirty region, candles
  incrementais (delta), backpressure de imagem (resize no host), e **soak test de
  72h** com rede instável cíclica e provider caindo aleatoriamente, validando os
  caminhos de recovery. Metas de CPU/heap definidas **após** medir a baseline no
  hardware real, não arbitradas antes.
- **Fase 11:** modularização — múltiplos providers de mercado atrás de
  `IMarketProvider` com fallback (a interface já permite; só ativar quando houver
  necessidade real, mantendo CoinGecko como padrão do MVP por ADR-0006);
  `NotificationService` com filas por prioridade; `ClockService` híbrido RTC↔NTP
  com degradação graciosa (ADR-0009), documentado na placa que de fato possuímos.

## Notas de escopo

- Candles incrementais, álbum/JPEG e multi-provider são **pós-MVP** (v1.0+),
  coerente com a ADR-0006 (MVP = snapshot CoinGecko 60s) e o `PLANEJAMENTO.md`.
  A arquitetura os permite; não se implementam no MVP.
- Háptico no `NotificationService` depende de existir motor háptico na placa —
  validar na Fase 0 antes de arquitetar para ele.
- `firmware/partitions.csv` e flags de hardware permanecem **preliminares** até a
  Fase 0 validar a placa real.
