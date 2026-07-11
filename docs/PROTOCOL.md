# NovaPainel - Protocolo

> **[SUPERSEDED — baseline v4, 2026-07-02]** Contratos de protocolo estão
> congelados junto com `shared/` (ADR-0003 v4) até existir o server (Fase 11).
> Mantido como histórico.

Os contratos trocados entre firmware e server (e o vocabulário de eventos
internos) ficam em **`/shared`**, que é a fonte única de verdade:

- Eventos do `EventBus`: `shared/protocol/events.md`
- Comandos: `shared/protocol/commands.md`
- Schemas (JSON Schema draft-07): `shared/schemas/`
  - `market_summary.schema.json`
  - `notification.schema.json`
  - `command.schema.json`
  - `device_state.schema.json` (contrato futuro - automação)
- Exemplos válidos: `shared/examples/`

Este arquivo é apenas um ponteiro para evitar duplicação. Ao alterar um contrato,
atualize `/shared` e os exemplos correspondentes.
