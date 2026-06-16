---
name: novapanel-schema-contract-sync
description: Keep NovaPanel shared contracts synchronized across shared/schemas, shared/examples, shared/protocol, firmware models, services, and docs. Use when changing JSON schemas, command/event protocol, market/weather/device payloads, examples, or C++ model fields that represent protocol data.
---

# NovaPanel Schema Contract Sync

Use this workflow when changing contracts.

## Read first

- `docs/PROTOCOL.md`
- `shared/README.md`
- relevant files in `shared/schemas/`
- relevant files in `shared/examples/`
- `shared/protocol/events.md` or `shared/protocol/commands.md`
- related C++ model in `firmware/components/models/include/app_state.hpp`

## Rules

- `shared/` is the source of truth for firmware/server/tool payload formats.
- Every schema change needs a matching example update.
- Every firmware model change that maps to a shared payload must be checked against the schema.
- Keep future contracts clearly labelled as future if not implemented.
- Avoid duplicate truth in docs; point to `shared/` for details.

## Checklist

1. Update schema.
2. Update example.
3. Check protocol docs.
4. Check firmware model fields and naming translations.
5. Check services/providers that produce or consume the payload.
6. Check docs that describe the contract.
7. If the change is architectural or changes scope, add an ADR.

## Common alignment checks

- C++ enum values versus JSON enum strings.
- `timestamp` in JSON versus `last_update_ms` or monotonic time in firmware.
- `source` and `stale` semantics for mock/cache/live data.
- Required JSON properties versus fields that may be unavailable offline.
