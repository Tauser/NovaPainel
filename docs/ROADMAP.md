# NovaPainel - Roadmap

> Estado atual: **Fase 0 quase fechada** (Gates 1-15 PASS, incl. soak de 8h em
> 2026-06-24 - só falta Gate 16/brownout-térmica) e **Fase 5 em andamento real**
> (não mais "a fazer"): `WaveshareBoard` + `CoinGeckoProvider` +
> `OpenMeteoProvider` + `SetupService` (NVS, `esp_wifi_connect`, NTP) +
> `WizardScreen` (onboarding completo, Wi-Fi/fuso/formato de hora) já rodam em
> `app_main.cpp` e foram validados na placa física, com pelo menos um bug real
> de hardware encontrado e corrigido (lock da LVGL travando a tela durante o
> wizard - ver `docs/DECISIONS.md`). `HomeScreen`/`MainShell` (redesign v2,
> ADR-0024) têm relógio, clima, mercado e USD/BRL dedicado (ADR-0026) reais;
> Agenda/Cenas rápidas/Player seguem placeholder (sem
> `CalendarService`/automação/áudio). **Fase 6 feita** (ADR-0027): cache em
> LittleFS via `CacheStore`, `cache_ready` real, dado sobrevive a
> reboot/sem-rede mostrado como "(cache)". **Fase 7 feita** (ADR-0028):
> partição de coredump + `SystemService` (motivo do reset, contador de
> reboots) + `SystemScreen` (tela própria, 8º ícone do rail) com idade do
> dado por domínio. **Fase 8 feita** (ADR-0029): circuit breaker por
> domínio (closed/open/half-open), backoff exponencial + jitter, probe
> automático via `update_clock()`, `is_degraded()` público — tudo dentro
> do `RequestOrchestrator`, serviços sem alteração. **Fase 9 feita**
> (ADR-0030): `sdkconfig.prod` com Secure Boot v2 + Flash Encryption +
> NVS Encryption + coredump off; `nvs_keys` em `partitions.csv`; zero
> mudanças de código-fonte. **Fase 10 feita** (ADR-0031): buffer de display
> em PSRAM quarter-height double-buffer (~600KB, ~4 flush/frame); dirty
> region via LVGL nativo; candles/JPEG pós-MVP. **Fase 11 feita** (ADR-0032):
> `NotificationService` fila prioritária cap-32 + `ClockService` comentários
> alinhados ao RTC interno ESP32-P4 confirmado; multi-provider pós-MVP.

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
Fase 4  - Display/touch/LVGL real (dirty rect nativo)               [feito, buffer PSRAM adiado p/ Fase 10]
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
- **Fase 4 (concluída):** LVGL real via `bsp_display_start()` (ADR-0018) - o
  BSP já cuida da `lvgl_task` própria, touch como indev e o lock
  (`IBoard::lock`/`unlock`, mapeado para `bsp_display_lock`/`unlock`)
  exigido por `UiDispatcher::process_pending()` ao chamar
  `HomeScreen::render`. `HomeScreen` virou widgets reais (relógio + mercado +
  status), escopo MVP - sem clima/agenda/player/cenas (essas dependem de
  providers/services que não existem ainda; ver `docs/design/README.md`).
  **Adiado para a Fase 10:** buffer de display full-frame/double-buffer em
  PSRAM (hoje usa o padrão do BSP - buffer parcial, single, RAM interna).
  Achado real (ADR-0018): o alocador padrão do LVGL (`LV_USE_BUILTIN_MALLOC`)
  reserva 64KB estáticos em RAM interna desde o boot, competindo com a
  inicialização do ESP-Hosted (que roda via `__attribute__((constructor))`
  antes do `app_main`) e causando falha na criação da própria main task:
  trocado para `LV_USE_CLIB_MALLOC` (alocação dinâmica via heap padrão).
  Tela de Boot/splash entra aqui também (`ScreenId::Boot` já existe em
  `AppState`, só falta o screen builder - ADR-0017).
- **Fase 5 (em andamento real):** `MockMarketProvider` já trocado por
  `CoinGeckoProvider` (REST, 60s, 6 req/min - ADR-0006/0021); `OpenMeteoProvider`
  para clima já existe (ADR-0023, Brasília fixo). Wizard de onboarding completo
  (nome de exibição, Wi-Fi com scan/lista de redes, fuso horário, formato de
  hora - ADR-0017/0020/0025) publica intenção via `EventBus`; `SetupService` é
  o único a persistir em NVS e chamar `esp_wifi_connect`/NTP (ADR-0017/0021/0022)
  - UI nunca persiste nem chama Wi-Fi direto. Mesmo `SetupService`/eventos serão
  reusados depois pela tela de Configurações. `MarketService`/`WeatherService`
  já checam `onboarding.wifi_status == Connected` antes de chamar o provider
  (corrigido - antes a 1a request saía antes do Wi-Fi associar). Wizard não
  tem passo de tema por decisão (`ThemeMode` fica no default até a futura
  tela de Configurações - ver `app_state.hpp`), não é uma lacuna.
  **Pendente:** fonte USD/BRL dedicada (hoje é a razão BTC/USD ÷ BTC/BRL do
  próprio CoinGecko, ver comentário em `coingecko_provider.cpp`).

## Detalhe das fases de hardening (pós-hardware provado)

- **Fase 6 (concluída):** cache em LittleFS com escrita atômica (`<key>.tmp`
  + `rename()`) e versionamento de schema por header (ADR-0015/0027) via
  `CacheStore` (`components/cache/`), montada na partição `storage`
  existente. `WeatherService`/`MarketService`/`ForexService` semeiam o
  `StateStore` a partir do cache no boot (`source=Cache, stale=true`) e
  gravam após cada fetch bem-sucedido; `HomeScreen` mostra "(cache)" pro
  clima e Bitcoin (Dólar já mostrava, ADR-0026). Sem migração de schema real
  ainda (não há v2 de nenhum schema pra migrar de - ver nota de escopo da
  ADR-0027); sinalização "sem-NTP" não foi implementada nesta fase.
- **Fase 7 (concluída):** partição `coredump` (256KB) + `ENABLE_TO_FLASH`
  (sem parsing on-device, ver ADR-0028). `SystemService` (reset reason +
  reboot count, padrão validado no harness Gate 15) + `SystemScreen`
  (tela própria, fora do `MainShell::content()` - motivo na ADR-0028),
  acessível pelo 8º ícone do rail. Idade do dado por domínio calculada de
  `last_update_ms` já existente, sem novo campo persistido.
- **Fase 8 (concluída):** circuit breaker por domínio no `RequestOrchestrator`
  — `CircuitState` (closed/open/half-open), backoff exponencial + jitter
  (XOR-shift PRNG, sem dep externa), `update_clock()` drive Open→HalfOpen,
  `is_degraded()` público para UI/serviços, backpressure via `can_request()`
  retornando `false`, retry finito por sonda limitada ao período de backoff
  (ADR-0029). Serviços e UI sem alteração.
- **Fase 9 (concluída):** `sdkconfig.prod` (Secure Boot v2 + Flash Encryption
  RELEASE + NVS Encryption + coredump off + log WARN), `nvs_keys` partition
  adicionada ao `partitions.csv`, `.gitignore` para chave de signing, comentário
  DEV/PROD em `sdkconfig.defaults`, auditoria de log de secrets confirmada
  (ADR-0030). Zero mudanças de código-fonte; DEV não afetado.
- **Fase 10 (concluída):** buffer de display migrado para PSRAM
  (`buff_spiram=true`, quarter-height double-buffer, ~600KB PSRAM, ~4 flush/frame
  vs ~24 antes) — bloqueador de Fase 4 (`LV_USE_BUILTIN_MALLOC`) já havia sido
  corrigido (ADR-0018/0031). Dirty region via LVGL nativo, sem mudança de código.
  Candles incrementais e backpressure de imagem seguem como pós-MVP (v1.0+).
  Soak test excluído por decisão de produto; pertence ao processo de release.
- **Fase 11 (concluída):** `NotificationService` — fila prioritária com cap 32,
  evicção de menor prioridade, `mark_read()`/`unread_count()`, `now_ms` em
  `notify()`. `ClockService` — comentários corrigidos para o RTC interno ESP32-P4
  com bateria 1220 confirmado em `HARDWARE.md`; lógica híbrida `kMinPlausibleEpoch`
  já estava correta. Multi-provider de mercado permanece pós-MVP (ADR-0032).

## Notas de escopo

- Candles incrementais, álbum/JPEG e multi-provider são **pós-MVP** (v1.0+),
  coerente com a ADR-0006 (MVP = snapshot CoinGecko 60s) e o `PLANEJAMENTO.md`.
  A arquitetura os permite; não se implementam no MVP.
- Háptico no `NotificationService` depende de existir motor háptico na placa —
  validar na Fase 0 antes de arquitetar para ele.
- `firmware/partitions.csv` e flags de hardware permanecem **preliminares** até a
  Fase 0 validar a placa real.
