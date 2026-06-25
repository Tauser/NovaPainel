# CLAUDE.md - NovaPainel

Guia para o Claude Code (e humanos) trabalhando neste repositório. Este arquivo
cobre **como** trabalhar no código.

**Ponto de entrada obrigatório para agentes:** leia `AGENTS.md` (skill routing)
e `skills/novapanel-firmware-workflow/SKILL.md` (workflow padrão e docs a ler)
antes de qualquer mudança. Para **escopo, requisitos e decisões**, a fonte de
verdade é `docs/` — leia `docs/PLANEJAMENTO.md` (base v3), `docs/ARCHITECTURE.md`,
`docs/ROADMAP.md` e `docs/DECISIONS.md` (ADRs) antes de mudanças relevantes.
Consulte `docs/HARDWARE.md` sempre que hardware, BSP, display, Wi-Fi ou ESP-Hosted
estiverem envolvidos.

## O que é o projeto

Smart display **local / offline-first** em **ESP32-P4** (ESP-IDF + C++),
organizado como monorepo. O firmware é o núcleo; o server é opcional/futuro.

```text
firmware/  núcleo do produto (ESP-IDF/C++)
server/    opcional, futuro - o firmware NUNCA depende dele (ADR-0002)
shared/    contratos firmware<->server (JSON Schemas + protocolo)
docs/      documentação canônica (v3) + ADRs
tools/     scripts (ex.: tools/scripts/host_check.sh)
```

Estado atual: **Fase 11 concluída** — MVP completo em produção.
`WaveshareBoard` real (display LVGL + touch + SD + ESP-Hosted/C6), Wi-Fi, NTP,
`CoinGeckoProvider` + `OpenMeteoProvider` + `ForexProvider` (APIs reais),
`CacheStore` (LittleFS), `SystemService`/`SystemScreen`, circuit breaker no
`RequestOrchestrator`, build PROD seguro (`sdkconfig.prod`), buffer de display
em PSRAM, `NotificationService` com fila prioritária. Próximas fases: v1.0
(modo noite, álbum, timer, perfis — Fase 12+).

## Regras de arquitetura (não-negociáveis)

```text
1. A UI não faz request direto. Services não tocam a UI.
2. Tudo passa por StateStore (estado) e EventBus (eventos).
3. Só a futura lvgl_task pode tocar objetos LVGL. Eventos de UI passam pelo
   UiDispatcher (ADR-0007).
4. Todo request externo passa pelo RequestOrchestrator (intervalo, prioridade,
   rate limit) - ADR-0004.
5. Hardware atrás de board/ (IBoard). APIs externas atrás de providers/.
6. Dados críticos funcionam offline (cache). Erros são tratados, não ignorados.
```

Fluxo de dados: `Provider -> Service -> StateStore -> EventBus -> UiDispatcher -> lvgl_task -> UI`.

## Layout do firmware

```text
firmware/components/
├─ core/       EventBus, StateStore, Service/ServiceManager,
│              RequestOrchestrator (circuit breaker, ADR-0012/0029),
│              UiDispatcher
├─ models/     AppState e structs (somente dados, sem lógica/IO -> testável no host)
├─ board/      IBoard + MockBoard (HAL) + WaveshareBoard (real, Fase 3)
├─ providers/  IMarketProvider + MockMarketProvider + CoinGeckoProvider
│              + OpenMeteoProvider + ForexProvider
├─ services/   ClockService (híbrido RTC↔NTP), MarketService, WeatherService,
│              ForexService, NotificationService (fila prioritária, cap-32),
│              SetupService (NVS + Wi-Fi + NTP), SystemService (reset reason)
├─ cache/      CacheStore (LittleFS, escrita atômica, ADR-0027)
├─ ui/         BootScreen, WizardScreen, HomeScreen, MainShell,
│              SystemScreen — todos LVGL real
└─ utils/      Result<T> / Status (erros sem exceções)
```

Cada componente é um componente ESP-IDF com `CMakeLists.txt` usando
`idf_component_register(SRCS ... INCLUDE_DIRS "include" REQUIRES ...)`.

## Convenções de código

- C++17, estilo simples e compatível com ESP-IDF. Sem overengineering.
- Namespace `nova` em todo o firmware.
- Headers em `include/`, fontes em `src/`. Um par .hpp/.cpp por unidade.
- Nomes: tipos `PascalCase`, métodos/variáveis `snake_case`, membros com sufixo
  `_` (ex.: `state_`). Logs com um `kTag` por arquivo via `ESP_LOGx`.
- Erros: retornar `Result<T>` / `Status` (utils/result.hpp). Evitar exceções.
- Sem `new`/`delete` cru quando der: objetos vivem no `app_main` (stack/estáticos)
  e são passados por referência.
- Comentários explicam o **porquê** e marcam o que é mock/futuro.

## Build e validação

ESP-IDF (alvo real):

```bash
cd firmware
idf.py set-target esp32p4
idf.py build
idf.py -p <porta> flash monitor
```

> A placa usa ESP32-P4 **sem Wi-Fi nativo**; rede depende do ESP32-C6 via
> ESP-Hosted/SDIO — validado na placa física (Gates 1-15 PASS, Fase 0).
> Build PROD: `idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.prod" build`
> (Secure Boot v2 + Flash Encryption + NVS Encryption — ver `sdkconfig.prod` e ADR-0030).

Sem ESP-IDF instalado, valide a lógica C++ no host (compila os componentes com
`g++` contra shims de esp_log/esp_timer/freertos):

```bash
bash tools/scripts/host_check.sh
```

Use o host_check após qualquer mudança em `core/`, `models/`, `services/`,
`providers/` ou `utils/`.

## Como adicionar coisas (atalhos)

Skills de projeto ficam em `skills/` (fonte versionada, veja `AGENTS.md` para
o mapeamento completo tarefa→skill). As mais usadas:

- `novapanel-add-service`  — novo Service (header+src+CMake+wiring no app_main)
- `novapanel-add-provider` — novo provider atrás de uma interface
- `novapanel-add-state-model` — novo estado/modelo em `models/`
- `novapanel-new-adr`      — registrar uma ADR em docs/DECISIONS.md
- `novapanel-host-check`   — rodar a validação host-compile

Ao adicionar um componente novo, lembre de: criar `include/` + `src/`, atualizar
o `CMakeLists.txt` do componente (e o `REQUIRES`), e registrar/instanciar no
`firmware/main/app_main.cpp`.

## Fora de escopo (não implementar)

TV Samsung (removido - ADR-0005). Sonoff/automação física, sensores, voz, câmera,
NoiseBot são **roadmap futuro** (ver docs/ROADMAP.md), não MVP. Mercado em tempo
real via WebSocket é futuro; no MVP é CoinGecko REST 60s / 6 req-min (ADR-0006).

## Contratos

Ao mudar um payload trocado entre partes, atualize `shared/schemas/*.schema.json`
e o exemplo em `shared/examples/`, e mantenha alinhado com os structs em
`firmware/components/models/`.

## Git

A pasta de trabalho pode estar em um mount com problemas de coerência para git;
se `git` se comportar de forma estranha aqui, rode os comandos git na máquina
local. Não commitar `firmware/build/`, `sdkconfig`, `__pycache__/` (ver
`.gitignore`).
