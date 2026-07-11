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
| Fase atual | **Fase 2 — Boot, display e shell de UI registrável** | 2026-07-11 |
| Firmware ativo | esqueleto v4 criado em `firmware/` (Fase 1) | 2026-07-11 |
| Hardware | validado (herdado da Fase 0 do baseline v3) | 2026 |
| Build PROD validado em bancada | não | — |
| OTA operacional | não | — |
| Soak (7 dias contínuos) no tree atual | não | — |

## Fases (espelho do ROADMAP — marcar apenas aqui)

```text
Fase 0  - Preparação do repositório e tooling            [concluída em 2026-07-11]
Fase 1  - Esqueleto arquitetural + board/ + CI host      [concluída em 2026-07-11]
Fase 2  - Boot, display, shell de UI registrável         [em andamento]
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

## Evidência de encerramento da Fase 0

- Firmware v3 arquivado em `reference/` sem builds, caches ou temporários; o
  tree rastreado fora de `reference/` mede 519.599 bytes.
- `bash tools/scripts/host_check.sh --tests` passou no Git Bash do Windows.
- Workflow CI passou no Linux para o commit `e1b1d59`:
  `https://github.com/Tauser/NovaPainel/actions/runs/29149942477`.
- Skills versionadas e `.gitignore` atualizados para o layout v4.

## Evidência de encerramento da Fase 1

- Esqueleto ESP-IDF v4 criado em `firmware/` com componentes `core`,
  `models`, `utils`, `board`, `providers` e `ui`.
- `bash tools/scripts/host_check.sh --app --tests` passou localmente com
  testes nativos de EventBus, StateStore, RequestOrchestrator, ActionQueue,
  MockBoard e ScreenRegistry.
- `idf.py build` passou localmente para `esp32p4` com ESP-IDF v5.5.4.
- Tabela A/B fixada em `firmware/partitions.csv` e registrada no ADR-0016.
- Gate `architecture_check.sh` adicionado para validar `app_main < 300` e
  dependências CMake/include da Fase 1 no CI.
- `RequestOrchestrator` cobre políticas default do `RESOURCE-BUDGET`, gap
  global de 400 ms, rate limit por minuto e circuit breaker.
- `core/` declara ownership/sincronização no código: `ActionQueue` protegido
  por mutex com log de overflow, `UiDispatcher` com máscara atômica e
  `StateStore` com acesso sincronizado.
- `app_main.cpp` tem 46 linhas, abaixo do limite de 300 linhas da fase.
- CI Linux passou para os commits de fechamento da fase:
  `https://github.com/Tauser/NovaPainel/actions/runs/29152773320`,
  `https://github.com/Tauser/NovaPainel/actions/runs/29158721839`,
  `https://github.com/Tauser/NovaPainel/actions/runs/29158876630` e
  `https://github.com/Tauser/NovaPainel/actions/runs/29159062385`.

## Dívidas conhecidas / riscos abertos

- Fase 2 ainda não possui validação de display real, shell UI registrável ou
  boot resiliente em bancada.
- Nenhuma evidência de estabilidade de longa duração em nenhum tree.
