---
name: novapanel-add-service
description: Scaffold or modify an internal NovaPanel firmware Service under firmware/components/services, including header/source, CMake registration, app_main wiring, StateStore/EventBus behavior, RequestOrchestrator usage, and validation. Use when adding WeatherService, CalendarService, AlarmService, PresenceService, or similar domain services.
---

# NovaPanel Add Service

Use this workflow to add a firmware service.

## Read first

- `skills/novapanel-firmware-workflow/SKILL.md`
- `docs/ARCHITECTURE.md`
- Existing examples:
  - `firmware/components/services/include/clock_service.hpp`
  - `firmware/components/services/src/clock_service.cpp`
  - `firmware/components/services/include/market_service.hpp`
  - `firmware/components/services/src/market_service.cpp`

## Confirm or infer

- Service name in PascalCase ending with `Service`.
- State it owns or updates.
- Collaborators by reference: usually `StateStore&`; add `RequestOrchestrator&` and provider interface only if it requests external data.
- `DataDomain` if it performs outbound requests.
- `StateStore` setter/event required, if new state is introduced.

## Rules

- Services never touch UI or LVGL.
- Services mutate state only through `StateStore`.
- Services never call providers without `orchestrator.can_request(domain)`.
- After a successful provider call, call `orchestrator.note_request(domain, now_ms)`.
- Keep service code in namespace `nova`.
- Header goes in `firmware/components/services/include/`.
- Source goes in `firmware/components/services/src/`.

## Steps

1. Add or update model/state first if the service needs new state.
2. Create the service header and source.
3. Add the source file to `firmware/components/services/CMakeLists.txt`.
4. Add required component dependencies to `REQUIRES`.
5. Wire the service in `firmware/main/app_main.cpp`.
6. Run `skills/novapanel-host-check/SKILL.md`.
7. Run ESP-IDF build when practical.

Do not add real network code here; put API logic in a provider.
