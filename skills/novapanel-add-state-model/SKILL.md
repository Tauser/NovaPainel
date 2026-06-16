---
name: novapanel-add-state-model
description: Add or modify NovaPanel AppState models, StateStore setters, EventBus events, UI render inputs, and related contracts. Use when adding weather, agenda, devices, sensors, cache status, market fields, system status, or any state shown by UI or consumed by services.
---

# NovaPanel Add State Model

Use this workflow when adding runtime state.

## Read first

- `firmware/components/models/include/app_state.hpp`
- `firmware/components/models/src/app_state.cpp`
- `firmware/components/core/include/state_store.hpp`
- `firmware/components/core/src/state_store.cpp`
- `firmware/components/core/include/event_bus.hpp`
- `firmware/components/core/src/event_bus.cpp`
- `shared/protocol/events.md`

## Steps

1. Add plain data structs to `AppState`; no I/O and no service logic.
2. Add an `EventType` if UI or services need to react to changes.
3. Add `to_string()` handling for new enum values.
4. Add a `StateStore` setter for mutations.
5. Publish the matching event from the setter.
6. Add the event to `shared/protocol/events.md`.
7. Update UI render code only to read from `StateStore`; do not add requests in UI.
8. Add schemas/examples in `shared/` only if the state is also a protocol payload.
9. Run host check and ESP-IDF build when practical.

## Naming guidance

- Use firmware field names in C++ style (`btc_usd`, `last_update_ms`).
- Use JSON schema names in API/protocol style (`btcUsd`, `timestamp`) when appropriate.
- Keep source/origin/staleness explicit for cached or mock data.
