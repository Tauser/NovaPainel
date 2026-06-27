# AGENTS.md - NovaPanel

## Visao do projeto

Este repositorio constroi o **NovaPanel**, um smart display **local /
offline-first** baseado em **ESP32-P4 + ESP32-C6**, com firmware em
**ESP-IDF + LVGL**, arquitetura modular e expansao futura para automacao,
sensores, midia, voz e integracao opcional com o ecossistema NoiseBot.

O painel deve continuar util mesmo sem `server/`, sem internet e sem depender
de um backend para as funcoes principais.

## Leitura correta do estado atual

Ao trabalhar neste repositorio, assuma sempre:

- `Fase 0` de hardware esta **concluida**
- a trilha `H` de consolidacao tecnica/documental esta **concluida**
- a `Fase 1` documental esta **concluida**
- o `firmware/` ativo esta em **reconstrucao funcional controlada**
- o `firmware_legacy_reference/` e **somente referencia para port seletivo**

Nao trate documentacao antiga ou implementacoes do tree legado como se ja
estivessem reentregues no `firmware/` novo.

## Fonte de verdade documental

Antes de alterar firmware, documentacao, contratos ou roadmap, leia:

- `skills/novapanel-firmware-workflow/SKILL.md`
- `docs/PLANEJAMENTO.md`
- `docs/ARCHITECTURE.md`
- `docs/ROADMAP.md`

Quando relevante, leia tambem:

- `docs/HARDWARE.md`
- `docs/DECISIONS.md`
- `docs/README.md`

Para operacao e release, consulte:

- `docs/SECURITY-OPERATIONS.md`
- `docs/FIELD-OPERATIONS.md`
- `docs/RELEASE-ROLLBACK.md`
- `docs/SOAK-VALIDATION.md`

## Principios obrigatorios

- O firmware e **offline-first** e nunca depende estruturalmente de `server/`.
- A UI **nao faz request direto**, nao persiste dados diretamente e nao toca
  hardware/rede por fora dos fluxos definidos.
- Toda mutacao de estado passa por `StateStore`.
- Todo request externo passa por `RequestOrchestrator`.
- Apenas a `lvgl_task` do BSP toca objetos LVGL; qualquer acesso externo exige
  o lock apropriado.
- Hardware e APIs entram por interfaces/modulos, nao por acoplamento cruzado.
- Quando houver falha externa, o comportamento esperado e degradar com clareza
  para cache/stale, nao travar a UX.

## Arquitetura esperada

Fluxo alvo:

```text
Provider -> Service -> StateStore -> EventBus -> UiDispatcher -> lvgl_task -> UI
```

Camadas esperadas:

- `components/core`
- `components/models`
- `components/board`
- `components/providers`
- `components/services`
- `components/ui`
- `shared`

Nao introduza atalhos arquiteturais que furam esse desenho sem registrar uma
decisao em `docs/DECISIONS.md`.

## Regras do baseline atual

- O `firmware/` novo e a unica base ativa de evolucao.
- O tree `firmware_legacy_reference/` serve para consulta, comparacao e port
  seletivo de comportamento ou estrutura.
- Nao reimporte o firmware antigo em bloco.
- Nao marque fase funcional como concluida sem que ela exista no `firmware/`
  ativo com validacao correspondente.
- Nao confunda patrimonio tecnico da `Fase 0`/trilha `H` com implementacao ja
  reentregue no reboot.

## Ordem oficial das proximas fases

Siga o roadmap consolidado:

- `Fase 2`: estabilizacao do reboot do firmware
- `Fase 3`: setup, conectividade e tempo
- `Fase 4`: dados reais e cache offline
- `Fase 5`: telas funcionais centrais do MVP
- `Fase 6`: observabilidade e resiliencia de operacao
- `Fase 7`: hardening de release e seguranca PROD

## Skills do projeto

As skills especificas do NovaPanel ficam em `skills/` e sao a fonte versionada
para qualquer agente trabalhando neste repositorio.

Use a skill especifica quando a tarefa envolver:

- fechar ou auditar fase: `skills/novapanel-close-phase/SKILL.md`
- hardware, ESP32-P4, C6 ou ESP-Hosted:
  `skills/novapanel-hardware-risk-gate/SKILL.md`
- build, flash, monitor, VS Code ou OpenOCD:
  `skills/novapanel-esp-idf-build-debug/SKILL.md`
- novo service: `skills/novapanel-add-service/SKILL.md`
- novo provider/API adapter: `skills/novapanel-add-provider/SKILL.md`
- novo estado/modelo: `skills/novapanel-add-state-model/SKILL.md`
- schemas, exemplos ou protocolo em `shared/`:
  `skills/novapanel-schema-contract-sync/SKILL.md`
- validacao C++ no host: `skills/novapanel-host-check/SKILL.md`
- nova decisao arquitetural: `skills/novapanel-new-adr/SKILL.md`

## Criterio de qualidade

Toda mudanca relevante deve buscar:

- escalabilidade de arquitetura
- robustez operacional
- confiabilidade em campo
- facilidade de manutencao
- documentacao coerente com o estado real do codigo

Se houver conflito entre “ser rapido” e “preservar a arquitetura”, prefira a
opcao que mantenha o sistema mais sustentavel.
