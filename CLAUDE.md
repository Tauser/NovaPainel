# CLAUDE.md — NovaPanel (baseline v4)

Guia para agentes e humanos trabalhando neste repositório. Este arquivo cobre
**como** trabalhar; ele NUNCA afirma o estado do projeto.

## Regra zero — onde está a verdade

- **Estado atual**: somente em `docs/STATUS.md`. Este arquivo, os ADRs e o
  roadmap nunca dizem "está pronto/em produção" — consulte o STATUS.
- **Escopo e produto**: `docs/PLANEJAMENTO.md`
- **Arquitetura alvo**: `docs/ARCHITECTURE.md` + `docs/UI-PATTERN.md`
- **Decisões**: `docs/DECISIONS.md` (ADRs) — mudou o desenho? registre ADR.
- **Limites físicos da placa**: `docs/RESOURCE-BUDGET.md` (contrato, não dica)
- **Fases e critérios de saída**: `docs/ROADMAP.md`
- **Testes/CI**: `docs/TESTING.md` · **Operação/release**: `docs/OPERATIONS.md`

Ponto de entrada obrigatório para agentes: `AGENTS.md`.

## O que é o projeto

Smart display **local / offline-first** em **ESP32-P4 + ESP32-C6**
(Waveshare ESP32-P4-WIFI6-Touch-LCD-7B, 7" 1024×600 MIPI-DSI), firmware
**ESP-IDF + LVGL em C++17**, monorepo:

```text
firmware/            núcleo do produto (criado na Fase 1 do baseline v4)
reference/           código v3 e legados — SOMENTE leitura para port seletivo
shared/              contratos firmware<->server (congelado até existir server)
docs/                documentação canônica v4
tools/               scripts (host_check portátil, CI)
skills/              workflows versionados para agentes
```

## Regras de arquitetura (não-negociáveis — detalhes no ARCHITECTURE.md)

```text
1. UI não faz request, não persiste, não toca hardware. UI lê estado e
   publica intenção.
2. Toda mutação de estado passa pelo StateStore; eventos pelo EventBus.
3. Só a lvgl_task toca objetos LVGL; acesso externo exige lock via board/.
4. Todo request externo passa pelo RequestOrchestrator e é executado pelo
   NetworkWorker (1 HTTPS por vez).
5. Hardware SÓ atrás de board/ (IBoard). APIs externas SÓ atrás de
   interfaces de provider. main/ é wiring — sem lógica, sem driver.
6. Telas entram pelo registro de ScreenSpec (docs/UI-PATTERN.md) — nunca
   por flags ou if/else no main.
7. Cada campo tocado por mais de uma task tem dono declarado ou é atômico
   (modelo de threads no ARCHITECTURE.md §Concorrência). "Parece seguro"
   não é análise.
8. Alocação segue o RESOURCE-BUDGET.md (o que vive em SRAM interna, o que
   vive em PSRAM, o que não pode coincidir com escrita de flash).
9. Dados críticos funcionam offline (cache com stale explícito). Erros
   viram degradação clara, nunca travamento.
```

## Convenções de código

- C++17, estilo ESP-IDF, sem overengineering. Namespace `nova`.
- Headers em `include/`, fontes em `src/`, um par .hpp/.cpp por unidade.
- Tipos `PascalCase`, métodos/variáveis `snake_case`, membros `sufixo_`.
- Erros por `Result<T>`/`Status` (`utils/`); sem exceções.
- Objetos de longa vida no `app_main` (stack/estático), passados por
  referência; sem `new`/`delete` cru.
- PROIBIDO `static std::function` global (estouro de `__cxa_atexit` já
  causou boot freeze no v3). Callbacks globais: ponteiro de função + contexto,
  ou estáticos locais de tipos triviais.
- Comentários explicam o porquê, com números quando houver medição.
- Logs: um `kTag` por arquivo via `ESP_LOGx`; nunca logar segredo.

## Build e validação

```bash
cd firmware
idf.py set-target esp32p4
idf.py build                     # DEV
idf.py -p <porta> flash monitor
bash tools/scripts/host_check.sh          # sempre após mudar core/models/
bash tools/scripts/host_check.sh --tests  # roda testes nativos
```

- host_check é portátil (bash puro); se falhar só no seu ambiente, o bug é
  do script — corrija o script, não pule a validação.
- Build PROD: ver `docs/OPERATIONS.md` (nunca aplicar Flash Encryption
  RELEASE fora do procedimento documentado — é irreversível).

## Definition of Done (qualquer mudança)

1. Compila no alvo (`idf.py build`) e no host (`host_check.sh`).
2. Testes nativos passam; mudança em provider exige fixture de payload.
3. Nenhuma regra de arquitetura furada sem ADR novo.
4. `docs/STATUS.md` atualizado se o estado do projeto mudou.
5. Nada de `build/`, `node_modules/`, temporários ou segredos no commit.

## Fora de escopo (não implementar sem ADR)

Server obrigatório, TV Samsung, voz, câmera, WebSocket de mercado em tempo
real. Automação/sensores são fases futuras do ROADMAP, não MVP.

## Git

Working tree pode estar em mount com coerência ruim para git; se `git` se
comportar estranho, rode os comandos na máquina local. Não commitar
`firmware/build/`, `sdkconfig` gerado, `node_modules/`, `__pycache__/`.
