---
name: novapanel-firmware-workflow
description: Core NovaPanel project workflow for firmware, docs, shared contracts, builds, roadmap, and repository work. Use whenever Codex or another agent works in D:\Projetos\NovaPanel, especially on ESP-IDF firmware, LVGL architecture, services, providers, StateStore/EventBus, hardware phases, or project documentation.
---

# NovaPanel Firmware Workflow

Use this skill as the default workflow for NovaPanel work.

## Read first

Read these before making changes:

- `AGENTS.md`
- `docs/ARCHITECTURE.md`
- `docs/ROADMAP.md`
- `docs/HARDWARE.md` when hardware, build target, BSP, display, touch, Wi-Fi, SD, PSRAM, RTC, or partitions are involved
- `docs/DECISIONS.md` when changing architecture or scope

## Project rules

- Keep the firmware offline-first. The firmware must not depend on `server/`.
- Keep `server/` optional and future unless the task explicitly targets it.
- Keep UI request-free. UI reads `StateStore`; it does not call providers or services.
- Mutate app state only through `StateStore`.
- Publish changes through `EventBus`.
- Route UI work through `UiDispatcher`; no task except the future `lvgl_task` may touch LVGL objects.
- Gate outbound requests through `RequestOrchestrator` before provider calls.
- Keep providers as adapters behind interfaces; services depend on interfaces, not concrete API clients.
- Use `Result<T>` / `Status`; do not add exceptions as control flow.
- Treat `firmware/partitions.csv` and hardware flags as preliminary until Fase 0 validates real hardware.

## Normal workflow

1. Inspect the current state with `git status --short --branch`.
2. Read the smallest relevant docs and source files.
3. Keep edits scoped to the phase and component touched.
4. Update docs/ADRs/contracts when behavior or architecture changes.
5. Validate:
   - Run `skills/novapanel-host-check/SKILL.md` workflow after C++ component changes when possible.
   - Run ESP-IDF build for firmware changes when possible:
     `cmd.exe /c "set IDF_PYTHON_ENV_PATH=C:\Espressif\tools\python\v5.5.4\venv&& call C:\esp\v5.5.4\esp-idf\export.bat && idf.py build"` from `firmware/`.
6. Report what changed, what was validated, and what remains intentionally future.

## Current baseline

- ESP-IDF: `v5.5.4`
- Target: `esp32p4`
- Main project dir for IDF commands: `firmware/`
- Current closed phase: Fase 2, firmware core with mocks
- Next hardware-heavy work: Fase 0 / Fase 3 risk gates and board/BSP validation
