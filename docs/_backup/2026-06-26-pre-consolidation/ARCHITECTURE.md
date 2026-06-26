# NovaPanel - Arquitetura

## Regra visual

Toda tela recriada no firmware deve consultar primeiro o protótipo LVGL v9.5 em
`docs/design/lvgl_export_reference/`. Esse export é a fonte visual de verdade
para `Boot`, `Setup`, `Home`, `Agenda`, `Mercado`, `Casa`, `Alarmes`, `Clima`,
`Timer`, `Configurações` e `Notificações`.

Se a tela ainda não existir no export, use o arquivo visual mais próximo como
base e registre o novo desenho ali antes de portar para `firmware/`.

## Fluxo de dados

```text
Provider → Service → StateStore → EventBus → UiDispatcher → lvgl_task → UI
```

- **Provider** abstrai uma fonte externa (API). Ex.: `MockMarketProvider` hoje,
  `CoinGeckoProvider` no futuro.
- **Service** orquestra a lógica de um domínio (relógio, mercado, notificações).
  Pede permissão ao `RequestOrchestrator` antes de qualquer request.
- **StateStore** guarda o estado único da aplicação (`AppState`). Toda mutação
  publica um evento no `EventBus`.
- **EventBus** entrega eventos de forma síncrona, na task de quem publicou.
- **UiDispatcher** assina os eventos relevantes para UI, enfileira e drena sob
  o lock do board (`IBoard::lock`/`unlock`) - a `lvgl_task` real é a do BSP
  (`bsp_display_start()`, Fase 4/ADR-0018), não uma task própria do firmware.
- **UI** (LVGL real desde a Fase 4) lê do `StateStore` e desenha.

## UI atual do firmware novo

O reboot do `firmware/` reorganizou a interface em shell + telas separadas:

- `novapanel_ui.cpp` monta o shell base com rail, topbar, dots e menu handle
- `screen_boot.cpp` e `screen_setup.cpp` continuam como telas próprias
- `screen_home.cpp` carrega a home principal
- `screen_agenda.cpp` já saiu do placeholder
- `screen_placeholders.cpp` mantém as demais telas ainda não portadas
- o mapeamento visual de cada tela deve seguir `docs/design/lvgl_export_reference/`

Regra prática: a tela que ainda não foi portada continua como placeholder até
ganhar correspondência direta no export visual.

## Regras de arquitetura

```text
- A UI não faz request direto.
- Services não mexem diretamente na UI.
- Tudo passa por StateStore/EventBus.
- UiDispatcher drena sob IBoard::lock()/unlock() antes de tocar LVGL (ADR-0018).
- RequestOrchestrator controla prioridade, frequência e rate limit.
- Board abstrai o hardware real (HAL), incluindo o lock de LVGL.
- Providers abstraem APIs externas.
- Nenhuma task fora da lvgl_task do BSP pode alterar objetos LVGL sem o lock.
```

## Camadas (componentes do firmware)

```text
main/                      app_main: wiring + loop (board real, resto mock)
components/
├─ core/                   EventBus, StateStore, Service/ServiceManager,
│                          RequestOrchestrator, UiDispatcher
├─ models/                 AppState e structs de dados (sem lógica/IO)
├─ board/                  HAL (IBoard) + MockBoard + WaveshareBoard (real,
│                          Fase 3 - ADR-0016; display/touch via BSP oficial,
│                          rede só sobe o link SDIO ao C6, sem AP ainda)
├─ providers/              IMarketProvider + MockMarketProvider
├─ services/               ClockService, MarketService, NotificationService
├─ ui/                     Shell LVGL + telas em `screens/` (boot, setup, home,
│                          agenda e placeholders)
└─ utils/                  Result<T> / Status
```

## UI marshaling (ADR-0007)

```text
Service Task
   ↓ publish
EventBus
   ↓ (eventos de UI)
UiDispatcher / UiEventQueue
   ↓ process_pending() roda na lvgl_task
lvgl_task
   ↓
Screen.render()
```

`UiDispatcher::process_pending()` coalesce os eventos pendentes e chama o
`render` callback (definido em `app_main.cpp`) sob `board.lock()`/`unlock()`,
que `HomeScreen::render` usa para construir/atualizar widgets LVGL reais.

## RequestOrchestrator (ADR-0004)

Porta única de saída. Cada `Service` chama `can_request(domain)` antes de acessar
um provider. Centraliza, por domínio: habilitado/pausado, intervalo, prioridade
e rate limit (ex.: `MarketSummary` = 60s, 6 req/min, Normal). `MarketRealtime` e
`MarketCandles` ficam desabilitados no MVP.

## Testabilidade

`core`, `models`, `services` e `utils` são escritos para depender o mínimo de
ESP-IDF, de modo a permitir testes no host (ver `firmware/tests/native`).
