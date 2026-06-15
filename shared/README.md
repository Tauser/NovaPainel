# NovaPainel - Shared

Contratos compartilhados entre `firmware/` e `server/` (e ferramentas). Fonte
única de verdade dos formatos trocados entre as partes.

- `schemas/` — JSON Schema (draft-07) dos payloads.
- `examples/` — um exemplo válido por schema.
- `protocol/` — descrição dos eventos e comandos.

Schemas atuais:

| Schema                     | Uso                                                        |
|----------------------------|------------------------------------------------------------|
| `market_summary.schema.json` | snapshot de mercado (BTC/USD-BRL) mostrado na Home/Mercado |
| `notification.schema.json`   | notificações do sistema/usuário                            |
| `command.schema.json`        | comando UI/server -> alvo (sistema, cena, dispositivo)     |
| `device_state.schema.json`   | estado de dispositivo (contrato futuro p/ automação)       |

> Alguns schemas (ex.: `device_state`, partes de `command`) descrevem
> funcionalidades de **roadmap futuro** (Sonoff/automação). Ficam aqui para
> estabilizar o contrato cedo, sem implementação correspondente ainda.
