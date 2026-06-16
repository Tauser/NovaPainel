---
name: novapanel-install-codex-skills
description: Install or sync the versioned NovaPanel project skills from the repository skills/ directory into the local Codex skills directory so they can be auto-discovered by any Codex agent. Use when the user wants these project skills available globally across threads or agents.
---

# NovaPanel Install Codex Skills

Use this when the user wants project skills available through Codex auto-discovery.

## Source and destination

- Source: `D:\Projetos\NovaPanel\skills\`
- Destination: `C:\Users\Tauser\.codex\skills\`

## Rules

- Treat `skills/` in the repo as the source of truth.
- Copy only `novapanel-*` skill folders.
- Do not delete unrelated skills from `.codex\skills`.
- Ask for approval before writing outside the workspace when required by sandbox permissions.

## Suggested command

From repo root in PowerShell:

```powershell
tools\scripts\install_codex_skills.ps1
```

After syncing, start a new Codex thread/session if skill metadata does not refresh immediately.
