---
name: novapanel-new-adr
description: Record a NovaPanel architecture decision in docs/DECISIONS.md with the next ADR number and project style. Use whenever a non-trivial technical, hardware, provider, storage, security, UI, protocol, or scope decision is made.
---

# NovaPanel New ADR

Use this workflow for non-trivial decisions.

## Read first

- `docs/DECISIONS.md`

## Inputs

Identify:

- short title
- decision
- motive and trade-off
- docs/code that must reflect the decision

## Steps

1. Find the highest existing `ADR-00NN`.
2. Append a new section with the next zero-padded number.
3. Match the existing style:

```markdown
## ADR-00NN - <Title>
**Decisão:** <what was decided>.
**Motivo:** <why; trade-offs>.
```

4. Write in Portuguese to match the project docs.
5. Update related docs if the decision changes scope, roadmap, protocol, or workflow.

## Rules

- Do not renumber existing ADRs.
- Do not rewrite old ADR history.
- If a decision is reversed, add a new ADR that supersedes the old one.
