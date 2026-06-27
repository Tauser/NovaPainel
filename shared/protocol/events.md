# Protocolo - Eventos

Eventos internos publicados no `EventBus` do firmware. A UI nunca é atualizada
diretamente por tasks de serviço: eventos relevantes para UI são marshalados
pelo `UiDispatcher` para a futura `lvgl_task` (ADR-0007).

| Evento                 | Quem publica                 | Significado                                  | Relevante p/ UI |
|------------------------|------------------------------|----------------------------------------------|:---------------:|
| `BootCompleted`        | app_main                     | Boot finalizado                              | não            |
| `BootStateChanged`     | StateStore                   | Progresso/mensagem do boot mudou             | sim            |
| `BootSkipRequested`    | StateStore/UI                | Usuario pediu pular a splash de boot         | não            |
| `ScreenChanged`        | StateStore                   | Tela ativa mudou (payload = ScreenId)        | sim            |
| `ClockUpdated`         | ClockService/StateStore      | Relógio avançou                              | sim            |
| `MarketUpdated`        | MarketService/StateStore     | Novo snapshot de mercado                     | sim            |
| `ForexUpdated`         | ForexService/StateStore      | Novo snapshot de câmbio                      | sim            |
| `NotificationCreated`  | NotificationService          | Nova notificação (payload = id)              | sim            |
| `RequestPolicyChanged` | RequestOrchestrator          | Política/prioridade por tela mudou           | não            |
| `ResourceWarning`      | (futuro) ResourceMonitor     | Memória/recurso baixo                        | sim            |
| `UiInvalidated`        | qualquer                     | Pede re-render da UI                         | sim            |
| `SystemStatusChanged`  | StateStore                   | Flags de board/display/touch/rede/cache      | sim            |

Fluxo:

```text
Service Task -> EventBus -> UiDispatcher (fila) -> lvgl_task -> Screen.render()
```
