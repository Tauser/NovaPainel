---
name: novapanel-close-phase
description: Audit and close a NovaPanel roadmap phase. Use when marking a phase as done, reviewing phase completion, updating docs/ROADMAP.md, validating build/test evidence, or deciding whether a phase is truly complete.
---

# NovaPanel Close Phase

Use this workflow before declaring any roadmap phase done.

## Inputs

Identify:

- phase number and title from `docs/ROADMAP.md`
- intended exit criteria
- files changed for the phase
- validation commands that prove the phase works

## Checklist

1. Read `docs/ROADMAP.md`, `docs/PLANEJAMENTO.md`, and any phase-specific docs.
2. Compare the phase description against actual source files.
3. Run `git status --short --branch` and separate project changes from local cache/editor files.
4. Validate the strongest practical signal for that phase:
   - firmware logic: host check and/or `idf.py build`
   - hardware phase: flash/monitor logs or explicit hardware checklist evidence
   - contracts: schemas and examples reviewed together
   - docs-only phase: docs are internally consistent
5. Update `docs/ROADMAP.md` only after validation.
6. Note intentionally deferred work by phase; do not let future work block closure.

## Required final assessment

Report:

- status: done, partially done, or not done
- evidence: exact commands/log summary or file checks
- remaining risks
- next recommended phase/checkpoint

Do not mark a phase as complete based only on intent or folder structure.
