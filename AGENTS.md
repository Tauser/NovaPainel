# AGENTS.md — NovaPanel (baseline v4)

## Visão do projeto

Smart display **local / offline-first** em **ESP32-P4 + ESP32-C6** com firmware
**ESP-IDF + LVGL (C++17)**. Central pessoal (hora, clima, mercado, agenda) que
continua útil sem servidor e sem internet. Expansão futura: automação,
sensores, mídia, voz, NoiseBot — por módulo, nunca por atalho entre camadas.

## Ordem de leitura obrigatória (antes de qualquer mudança)

1. `docs/STATUS.md` — **único** arquivo que diz o estado atual. Nenhum outro
   documento (nem este) é fonte de estado. Divergência = bug de doc.
2. `CLAUDE.md` — regras de trabalho, invariantes, Definition of Done.
3. `docs/ARCHITECTURE.md` — desenho alvo. Para UI: `docs/UI-PATTERN.md`.
4. `docs/ROADMAP.md` — em que fase a mudança se encaixa e o critério de saída.
5. Se envolver hardware, memória, rede ou display: `docs/HARDWARE.md` e
   `docs/RESOURCE-BUDGET.md` (contrato físico da placa — violar = regressão).
6. Se envolver decisão estrutural: `docs/DECISIONS.md` (registrar ADR novo).

## Leitura correta do baseline

- O baseline vigente é o **v4** (reconstrução total decidida em 2026-07-02,
  ADR-0001 v4). Código anterior é referência de port em `reference/`
  (até a Fase 0 concluir a movimentação, pode ainda estar em `firmware/` —
  confirme no STATUS).
- Código de referência é **somente leitura**: portar seletivamente com crítica,
  nunca copiar wiring, god-files ou padrões vetados (ver ADRs 0002–0012 v4).
- Não trate documento histórico ou código v3 como se fosse o estado atual.

## Princípios obrigatórios (resumo executável)

- UI lê estado e publica intenção; nunca faz request/persistência/hardware.
- Toda mutação passa por `StateStore`; todo request por `RequestOrchestrator`
  + `NetworkWorker`.
- Só a `lvgl_task` toca LVGL; fora dela, lock via `board/`.
- Hardware atrás de `IBoard`; API externa atrás de interface de provider;
  tela nova via registro de `ScreenSpec`.
- Concorrência: campo compartilhado entre tasks tem dono declarado ou é
  atômico — comentário dizendo "seguro" não substitui análise.
- Falha externa degrada para cache/stale com sinalização clara na UI.
- Fechou fase ou mudou estrutura? Atualize `docs/STATUS.md` no mesmo commit.

## Skill routing (tarefa → skill em `skills/`)

- Workflow padrão de mudança de firmware → `novapanel-firmware-workflow`
- Novo service → `novapanel-add-service`
- Novo provider (sempre com interface + fixture) → `novapanel-add-provider`
- Novo estado/modelo → `novapanel-add-state-model`
- Nova tela → seguir `docs/UI-PATTERN.md`
- Registrar decisão → `novapanel-new-adr`
- Validação host → `novapanel-host-check`
- Fechar fase (inclui sync do STATUS) → `novapanel-close-phase`

> Skills herdadas do v3 podem referenciar caminhos antigos; ao usá-las,
> verifique contra este documento e corrija a skill se divergir.

## Proibições que já causaram incidentes reais (v3)

- `static std::function` global → overflow de `__cxa_atexit` → boot freeze.
- `bsp_audio_init(nullptr)` → crash + coredump corrompido + boot loop.
- Mais de 1 handshake TLS simultâneo → esgotamento de SRAM interna.
- Escrita de flash (NVS/LittleFS) em rajada durante render → glitch no DSI.
