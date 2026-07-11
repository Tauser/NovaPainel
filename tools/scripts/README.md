# tools/scripts

- `host_check.sh` - gate Bash portátil. Na Fase 0 valida o tooling sem
  firmware ativo; a partir da Fase 1 compila as fontes listadas em
  `firmware/tests/host_sources.txt`, executa o runner nativo com `--tests` e
  pode syntax-checkar `main/app_main.cpp` com `--app`.
- `ci_hygiene.sh` - bloqueia artefatos gerados, arquivos grandes fora de
  `reference/` e padrões básicos de segredo em conteúdo rastreado.
- `idf_env.ps1` - ativa o ambiente ESP-IDF v5.5.4 (PowerShell) e entra em
  `firmware/`. Precisa ser dot-sourced: `. tools\scripts\idf_env.ps1`.
  Depois disso, `idf.py build` / `idf.py -p COM8 flash` / `esptool.py`
  funcionam direto.
- `install_codex_skills.ps1` - copia as skills `novapanel-*` para o Codex.

Placeholder para ideias futuras:

- validar `shared/examples/*.json` contra `shared/schemas/*.schema.json`;
- gerador de cabeçalhos a partir dos schemas (futuro).
