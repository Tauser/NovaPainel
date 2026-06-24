# NovaPainel - Arquitetura

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
├─ ui/                     HomeScreen (LVGL real, Fase 4 - relógio+mercado+status)
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
