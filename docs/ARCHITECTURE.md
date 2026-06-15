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
- **UiDispatcher** assina os eventos relevantes para UI, enfileira e drena na
  futura `lvgl_task`.
- **UI** (LVGL, futuro) lê do `StateStore` e desenha.

## Regras de arquitetura

```text
- A UI não faz request direto.
- Services não mexem diretamente na UI.
- Tudo passa por StateStore/EventBus.
- UiDispatcher é obrigatório para marshaling futuro para a lvgl_task.
- RequestOrchestrator controla prioridade, frequência e rate limit.
- Board abstrai o hardware real (HAL).
- Providers abstraem APIs externas.
- Nenhuma task além da lvgl_task pode alterar objetos LVGL.
```

## Camadas (componentes do firmware)

```text
main/                      app_main: wiring + loop (mock)
components/
├─ core/                   EventBus, StateStore, Service/ServiceManager,
│                          RequestOrchestrator, UiDispatcher
├─ models/                 AppState e structs de dados (sem lógica/IO)
├─ board/                  HAL (IBoard) + MockBoard
├─ providers/              IMarketProvider + MockMarketProvider
├─ services/               ClockService, MarketService, NotificationService
├─ ui/                     HomeScreen (logs hoje; LVGL depois)
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

No esqueleto atual não há LVGL: `UiDispatcher::process_pending()` coalesce os
eventos pendentes e chama um `render` que imprime a Home via logs. O contrato
(quem pode tocar a UI e quando) já é o definitivo.

## RequestOrchestrator (ADR-0004)

Porta única de saída. Cada `Service` chama `can_request(domain)` antes de acessar
um provider. Centraliza, por domínio: habilitado/pausado, intervalo, prioridade
e rate limit (ex.: `MarketSummary` = 60s, 6 req/min, Normal). `MarketRealtime` e
`MarketCandles` ficam desabilitados no MVP.

## Testabilidade

`core`, `models`, `services` e `utils` são escritos para depender o mínimo de
ESP-IDF, de modo a permitir testes no host (ver `firmware/tests/native`).
