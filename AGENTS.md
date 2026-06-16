## Imported Claude Cowork project instructions

Este projeto tem como objetivo construir um **Smart Display local/offline-first** usando o **ESP32-P4-WIFI6-Touch-LCD-7B**, com firmware em **ESP-IDF + LVGL**, arquitetura modular em serviços internos e integração com dispositivos da casa.

O painel deve funcionar como uma central pessoal, financeira e de automação, mantendo as funções principais disponíveis mesmo sem depender de um servidor externo.

## Skills do projeto

As skills especificas do NovaPanel ficam em `skills/` e sao a fonte versionada
para qualquer agente trabalhando neste repositorio.

Antes de alterar firmware, documentacao, contratos ou roadmap, leia:

- `skills/novapanel-firmware-workflow/SKILL.md`

Use tambem a skill especifica quando a tarefa envolver:

- fechar ou auditar fase: `skills/novapanel-close-phase/SKILL.md`
- hardware, ESP32-P4, C6 ou ESP-Hosted: `skills/novapanel-hardware-risk-gate/SKILL.md`
- build, flash, monitor, VS Code ou OpenOCD: `skills/novapanel-esp-idf-build-debug/SKILL.md`
- novo service: `skills/novapanel-add-service/SKILL.md`
- novo provider/API adapter: `skills/novapanel-add-provider/SKILL.md`
- novo estado/modelo: `skills/novapanel-add-state-model/SKILL.md`
- schemas, exemplos ou protocolo em `shared/`: `skills/novapanel-schema-contract-sync/SKILL.md`
- validacao C++ no host: `skills/novapanel-host-check/SKILL.md`
- nova decisao arquitetural: `skills/novapanel-new-adr/SKILL.md`
