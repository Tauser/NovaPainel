# tools/scripts

- `host_check.sh` - compila core/models/services/providers/utils (e
  opcionalmente `main/app_main.cpp` com `--app`) no host via g++, sem
  ESP-IDF. Rodar depois de qualquer mudança nesses diretórios.
- `idf_env.ps1` - ativa o ambiente ESP-IDF v5.5.4 (PowerShell) e entra em
  `firmware/`. Precisa ser dot-sourced: `. tools\scripts\idf_env.ps1`.
  Depois disso, `idf.py build` / `idf.py -p COM8 flash` / `esptool.py`
  funcionam direto.
- `install_codex_skills.ps1` - copia as skills `novapanel-*` para o Codex.

Placeholder para ideias futuras:

- validar `shared/examples/*.json` contra `shared/schemas/*.schema.json`;
- gerador de cabeçalhos a partir dos schemas (futuro).
