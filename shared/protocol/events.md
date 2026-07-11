# Protocolo - Eventos

Eventos internos publicados no `EventBus` do firmware. A UI nunca é atualizada
diretamente por tasks de serviço: eventos relevantes para UI são marshalados
pelo `UiDispatcher` para a futura `lvgl_task` (ADR-0007).

| Evento           | Quem publica            | Significado                                  | Relevante p/ UI |
|------------------|-------------------------|----------------------------------------------|:---------------:|
| `ClockChanged`   | StateStore              | Snapshot de relógio mudou                    | sim             |
| `MarketChanged`  | StateStore              | Snapshot de mercado/câmbio mudou             | sim             |
| `WeatherChanged` | StateStore              | Snapshot de clima mudou                      | sim             |
| `SystemChanged`  | StateStore              | Flags de sistema/board/rede/cache mudaram    | sim             |
| `ResourceWarning`| SystemService futuro    | Memória/recurso abaixo do limiar operacional | sim             |
| `UiAction`       | UI/ActionQueue futura   | Intenção do usuário para services/store      | não             |

Fluxo:

```text
Service Task -> EventBus -> UiDispatcher (fila) -> lvgl_task -> Screen.render()
```
