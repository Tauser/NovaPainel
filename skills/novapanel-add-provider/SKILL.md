---
name: novapanel-add-provider
description: Scaffold or modify a NovaPanel provider/API adapter under firmware/components/providers behind an interface, using Result<T>/Status and service injection. Use when adding CoinGeckoProvider, OpenMeteoProvider, ForexProvider, mock providers, or any external data source adapter.
---

# NovaPanel Add Provider

Use this workflow to add an external data adapter.

## Read first

- `skills/novapanel-firmware-workflow/SKILL.md`
- `docs/ARCHITECTURE.md`
- Relevant ADR in `docs/DECISIONS.md`
- Existing examples:
  - `firmware/components/providers/include/i_market_provider.hpp`
  - `firmware/components/providers/include/mock_market_provider.hpp`
  - `firmware/components/providers/src/mock_market_provider.cpp`

## Rules

- Services depend on provider interfaces, not concrete providers.
- Providers perform I/O and parsing only.
- Scheduling, rate limiting, priority, and request permission belong to services plus `RequestOrchestrator`.
- Return `Result<T>` or `Status`.
- Keep parsing defensive; map failures to meaningful `ErrorCode`.
- Keep providers in namespace `nova`.

## Steps

1. Add or reuse an interface header, such as `i_weather_provider.hpp`.
2. Add any required model structs in `firmware/components/models/`.
3. Add matching `shared/schemas/` and `shared/examples/` only if the payload crosses firmware/server/tool boundaries.
4. Implement a mock provider first when the real network phase has not started.
5. Add the provider source to `firmware/components/providers/CMakeLists.txt`.
6. Inject the provider into the consuming service by interface reference.
7. Run host check and ESP-IDF build when practical.

## Real network providers

For HTTPS/WebSocket providers, confirm Fase 0 ESP-Hosted/Wi-Fi risk gates first. Do not assume networking works just because the provider compiles.
