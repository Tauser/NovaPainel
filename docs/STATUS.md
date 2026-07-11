# NovaPanel — STATUS (fonte única de estado)

> **Este é o ÚNICO arquivo do repositório autorizado a afirmar o estado atual
> do projeto.** Todos os outros documentos descrevem alvo, política ou
> história — nunca estado. Se qualquer outro arquivo (inclusive `CLAUDE.md`)
> contradisser este, este vence e o outro está com bug.
>
> Regra de atualização: todo fechamento de fase, milestone ou mudança de
> estrutura do repositório DEVE atualizar este arquivo no mesmo commit.

## Estado geral

| Item | Estado | Desde |
|---|---|---|
| Baseline vigente | **v4 (reconstrução total)** | 2026-07-02 |
| Fase atual | **Fase 0 — Preparação do repositório** | 2026-07-02 |
| Firmware ativo | inexistente (será criado em `firmware/` na Fase 1) | — |
| Hardware | validado (herdado da Fase 0 do baseline v3) | 2026 |
| Build PROD validado em bancada | não | — |
| OTA operacional | não | — |
| Soak (7 dias contínuos) no tree atual | não | — |

## Fases (espelho do ROADMAP — marcar apenas aqui)

```text
Fase 0  - Preparação do repositório e tooling            [em andamento]
Fase 1  - Esqueleto arquitetural + board/ + CI host      [pendente]
Fase 2  - Boot, display, shell de UI registrável         [pendente]
Fase 3  - Conectividade, tempo e onboarding              [pendente]
Fase 4  - Dados reais, cache offline e degradação        [pendente]
Fase 5  - Telas funcionais do MVP                        [pendente]
Fase 6  - Observabilidade, soak e resiliência provada    [pendente]
Fase 7  - OTA + release PROD seguro                      [pendente]
Fase 8+ - v1.0 e extensões (ver ROADMAP)                 [futuro]
```

## Patrimônio herdado do baseline v3 (não refazer, portar)

- Hardware, BSP, display MIPI-DSI, touch, ESP-Hosted P4↔C6: **provados**.
- Conhecimento de plataforma consolidado em `docs/RESOURCE-BUDGET.md` e
  `docs/HARDWARE.md` (contenção MSPI, TLS, SRAM interna).
- Código v3 disponível para port seletivo em `reference/firmware_v3/` e no
  snapshot de bancada `reference/firmware_v3_field_snapshot/`; ambos foram
  preservados sem artefatos gerados em 2026-07-11.
- ADRs históricos do v3: preservados no histórico git (`docs/DECISIONS.md`
  anterior a 2026-07-02).

## Dívidas conhecidas / riscos abertos

- Reescrever `host_check.sh` em bash portátil e ativar o CI Linux da Fase 0.
- Nenhuma evidência de estabilidade de longa duração em nenhum tree.
