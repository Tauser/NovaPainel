# NovaPanel — Arquitetura (baseline v4)

> Arquitetura ALVO do firmware. Este documento não descreve estado — ver
> `STATUS.md`. Incorpora as lições do baseline v3 (o que funcionou entra como
> contrato; o que falhou entra como proibição). Decisões: `DECISIONS.md`.

## 1. Objetivos de arquitetura

- offline-first real;
- baixo acoplamento entre UI, domínio, hardware e rede — **verificável pela
  topologia de dependências (CMake REQUIRES), não por disciplina**;
- crescimento por módulos e por registro (telas, providers, domínios);
- concorrência com propriedade declarada, não por convenção;
- respeito estrutural ao orçamento físico da placa (`RESOURCE-BUDGET.md`);
- debug, teste e operação de campo viáveis.

## 2. Contexto do sistema

```text
Usuário ─ touch/display ─┐
APIs externas (opcionais)─┤→ NovaPanel Firmware → NVS + cache local
server/ (opcional futuro)─┘        (P4 + C6)      coredump + partições OTA
```

Regras de contexto: o firmware continua útil sem server e sem internet;
qualquer integração futura preserva esse contrato.

## 3. Fluxo principal de dados (sem exceções)

```text
Provider → Service → StateStore → EventBus → UiDispatcher → lvgl_task → UI
   ↑                                                            │
   └────────── RequestOrchestrator ← NetworkWorker ←────────────┘
                (política)            (execução)      intenção da UI volta por
                                                      ActionQueue → Service/Store
```

- `Provider` fala com API/filesystem/hardware especializado e traduz payload
  externo em modelo interno. Não conhece UI nem StateStore.
- `Service` aplica regra de negócio, decide persistência/cache e escreve no
  `StateStore`. Não toca LVGL.
- `StateStore` é o estado canônico; toda mutação passa por ele.
- `EventBus` propaga sinalização (tipo + int32); dados grandes viajam pelo
  StateStore, nunca pelo evento.
- `UiDispatcher` enfileira e coalesce eventos relevantes para a UI — é o
  ÚNICO dono de coalescing (proibido re-coalescer em main/; lição v3).
- Só a `lvgl_task` toca objetos LVGL.

## 4. Camadas e componentes

```text
main/            wiring puro: monta o grafo, registra telas/fetchers, loop.
                 PROIBIDO: driver, lógica de domínio, política de repintura.
components/
  core/          EventBus, StateStore, UiDispatcher, RequestOrchestrator,
                 ServiceManager, ActionQueue
  models/        AppState e structs puros (sem IO) — host-checkáveis
  board/         IBoard + WaveshareBoard + MockBoard (HAL real, ver §5)
  providers/     interfaces (IMarketProvider, IWeatherProvider, IForexProvider)
                 + adaptadores concretos (CoinGecko, OpenMeteo, AwesomeAPI)
  services/      Clock, Setup, System, Notification, Market, Forex, Weather,
                 NetworkWorker, Audio
  cache/         CacheStore (LittleFS, blobs versionados, escrita atômica)
  ui/            shell + registro de telas (ScreenSpec) + view-models
                 (padrão obrigatório: UI-PATTERN.md)
  utils/         Result<T>/Status, http_client, helpers puros
```

Regras de dependência (impostas via CMake `REQUIRES` e revisadas em review):

- `ui/` → só `models`, `lvgl`, `core` (dispatcher/action). Nunca services,
  providers, board, rede ou filesystem.
- `services/` → core, models, cache, interfaces de provider e de board.
  Nunca adaptador concreto de provider (injeção no main).
- `providers/` → models, utils. Nunca core, ui, board.
- `board/` → BSP/drivers. Nunca core, services, ui.
- `main/` → tudo, mas só para construir e conectar.

## 5. `board/` — HAL obrigatória (correção estrutural do v3)

O v3 abandonou a HAL e o BSP vazou para o main (incl. ~300 linhas de driver
de áudio). No v4, `IBoard` é pequena e real:

```cpp
class IBoard {
 public:
  virtual bool init_display() = 0;          // falha NÃO aborta: ver §9 boot
  virtual bool lock_ui(uint32_t timeout_ms) = 0;   // lock da lvgl_task
  virtual void unlock_ui() = 0;
  virtual bool lock_shared_i2c(uint32_t timeout_ms) = 0;  // ver nota abaixo
  virtual void unlock_shared_i2c() = 0;
  virtual void set_brightness(int pct) = 0;
  virtual IAudio* audio() = 0;              // nullptr se indisponível
  // ...
};
```

**Nota crítica (invariante físico herdado do v3):** nesta placa, GT911
(touch) e ES8311 (codec) compartilham o barramento I2C, e o polling de touch
roda dentro do display lock. `lock_shared_i2c()` PODE ser implementado sobre
o display lock — mas o resto do sistema só conhece o nome semântico. O
invariante vive em UM lugar (WaveshareBoard), não em comentários espalhados.

`MockBoard` permite host-check e testes de service/UI-logic sem hardware.
Áudio é um módulo (`services/audio` sobre `IAudio`), nunca código inline.

## 6. Modelo de concorrência (propriedade declarada)

Tasks do sistema e o que cada uma possui:

| Task | Dono de | Pode |
|---|---|---|
| `lvgl_task` (BSP) | objetos LVGL, render, touch | ler snapshots do StateStore |
| `main_loop` (app_main, tick 200 ms) | ActionQueue drain, ServiceManager.tick, UiDispatcher.process_pending, escrita nas telas (sob lock_ui) | mutar StateStore |
| `net_worker` | execução HTTP (1 por vez), refresh de services de dados | mutar StateStore (thread-safe) |
| ISR/driver callbacks (Wi-Fi etc.) | nada | só setar `std::atomic` drenado no tick |

Regras obrigatórias:

1. Todo campo de classe acessado por mais de uma dessas tasks é `std::atomic`,
   protegido por mutex, ou trafega pela ActionQueue. **O header declara o dono
   de cada campo compartilhado.** Comentário "seguro chamar de qualquer task"
   sem mecanismo é proibido (bug real do v3 em `request_ohlc_period`).
2. EventBus é síncrono: handler roda na task do publicador. Handler de
   subscriber NÃO pode tocar LVGL nem bloquear; se precisar, posta na
   ActionQueue ou no UiDispatcher.
3. ActionQueue: profundidade dimensionada (≥16), **overflow loga e conta em
   métrica** — nunca drop silencioso (bug real do v3, fila de 4 com drop mudo).
4. Render nunca bloqueia por service lento; operação longa nunca roda em
   callback de toque.
5. Prioridades de task documentadas aqui quando fixadas (net_worker < lvgl).

## 7. Estado e contratos

`AppState` é a fonte única de verdade. Categorias: navegação, clock, mercado,
forex, clima, sistema, notificações, preferências, onboarding.

- Todo domínio de dados carrega `valid`, `stale`, `source`
  (`Live|Cache|Mock`) e `last_update_ms` — a UX distingue dado real, cache e
  ausência.
- Snapshot é cópia; dados grandes (OHLC, futura agenda/fotos) ficam FORA do
  snapshot, com acessor dedicado (padrão validado no v3).
- Leituras frequentes usam acessores granulares (`clock()`, `weather()`), não
  o snapshot inteiro.
- Payload trocado com partes externas: atualizar `shared/schemas/` + exemplo
  + struct em `models/` juntos (enquanto `shared/` estiver congelado, não
  criar contrato novo).

## 8. Requests e resiliência

`RequestOrchestrator` (política) + `NetworkWorker` (execução) — separação
validada em campo no v3 (ADR-0004 v4):

- por domínio: enable, prioridade, intervalo mínimo, rate limit, circuit
  breaker (Closed→Open→HalfOpen) com backoff exponencial + jitter;
- execução serializada: **1 HTTPS por vez em todo o firmware**, gap entre
  buscas, escalonamento natural no boot;
- corpo HTTP em SRAM interna com cap (ver RESOURCE-BUDGET); truncamento é
  FALHA do request (conta no breaker), não warning silencioso;
- contrato de UX: rede caiu → UI operável; API caiu → cache/stale; sem dado
  algum → comunicação clara.

## 9. Boot, recovery e operação contínua

- Ordem: NVS → board.init_display → shell UI mínima → rede → services →
  dados de cache → dados live.
- **Falha de display não chama `abort()`** (bug v3): retry com backoff; após
  N falhas, reboot com breadcrumb persistido em NVS e contador que degrada a
  cadência (evita boot loop quente).
- Reset reason + reboot count entram no estado na inicialização.
- Watermarks de heap (interno e PSRAM) amostrados periodicamente pelo
  SystemService; cruzou limiar → evento `ResourceWarning` (que TEM handler:
  log + métrica persistida + modo focado desligado).
- Coredump para partição flash habilitado inclusive em PROD
  (política: OPERATIONS.md).

## 10. Persistência

- **NVS**: Wi-Fi, preferências, flags operacionais, breadcrumbs. Schema
  versionado com migração; versão futura desconhecida → ignora persistido,
  nunca bricka. Escrita deduplicada (não regravar valor idêntico — erase de
  flash causa glitch no DSI, ver RESOURCE-BUDGET).
- **LittleFS (cache)**: blobs binários com header magic/versão/tamanho,
  escrita tmp+rename atômica, mismatch → descarte silencioso. Persistência
  throttlada (dados de UI ficam no StateStore; flash só a cada intervalo
  longo).

## 11. UI

Regida por `UI-PATTERN.md` (documento-dono). Resumo dos contratos:

- Tela = `ScreenSpec` registrado: `{id, build(parent, vm), update(vm),
  invalidation_mask}` — o shell itera o registro; **main/ não conhece telas**.
- Invalidação por máscara de eventos declarada pela tela, não por flags no
  main (bug estrutural v3).
- View-model por tela: a tela recebe struct de apresentação pronta (strings
  formatadas, cores decididas), não o AppState cru.
- Strings de produto centralizadas (`ui/strings_ptbr.hpp`) — i18n futura
  barata sem requisito de i18n agora.
- Proibido `static std::function` global (boot freeze v3); handles LVGL como
  estáticos triviais ou membros de contexto da tela.
- Repintura alvo < 100 ms por tela; atualização granular por widget quando o
  custo justificar (medir antes de otimizar).

## 12. Testabilidade

Pirâmide (detalhes em `TESTING.md`): host/unit para core+models+lógica de
service (via MockBoard/providers fake) → fixtures de payload para parsing de
provider → build IDF → validação em placa → soak. Comportamento de degradação
tem teste explícito. `host_check.sh` é portátil (bash puro, roda em CI Linux).

## 13. Extensibilidade

Sensores, automação, álbum, voz, server: entram por módulo novo (provider +
service + tela registrada), declarando custo contra o RESOURCE-BUDGET antes
de entrar no roadmap. Extensão que exige furar camada → ADR primeiro.
