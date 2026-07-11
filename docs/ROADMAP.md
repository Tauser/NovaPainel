# NovaPanel — Roadmap (baseline v4)

> Fases da reconstrução total decidida em 2026-07-02 (ADR-0001 v4). Este
> documento define ENTREGAS e CRITÉRIOS DE SAÍDA; o andamento é marcado
> exclusivamente em `STATUS.md`. Numeração única — proibido citar numerações
> de baselines anteriores.

## Princípios de execução

- Nenhuma fase conta como entregue sem TODOS os critérios de saída atendidos
  e `STATUS.md` atualizado no mesmo commit (skill `novapanel-close-phase`).
- `reference/firmware_v3/` é insumo de port seletivo: portar módulo a módulo
  com crítica; proibido copiar wiring, god-files ou padrões vetados nos ADRs.
- Cada fase religa código + docs + validação. Otimizar antes da base voltar
  é perda de foco; MAS os contratos estruturais (board/, ScreenSpec,
  interfaces de provider, ownership de threads) entram DESDE a Fase 1 —
  a lição do v3 é que "estrutura depois" nunca chega.

## Visão geral

```text
Fase 0  - Preparação do repositório e tooling
Fase 1  - Esqueleto arquitetural + board/ + CI host
Fase 2  - Boot, display e shell de UI registrável
Fase 3  - Conectividade, tempo e onboarding
Fase 4  - Dados reais, cache offline e degradação
Fase 5  - Telas funcionais do MVP
Fase 6  - Observabilidade, soak e resiliência provada
Fase 7  - OTA + release PROD seguro          ← fim do MVP
Fase 8  - v1.0 Painel Pessoal (noite, timer, álbum, agenda, perfis)
Fase 9  - Sensores e automação local
Fase 10 - Mídia e experiências avançadas
Fase 11 - Ecossistema opcional (server, voz, NoiseBot)
```

## Fase 0 — Preparação do repositório e tooling

Entregas: mover firmware v3 para `reference/firmware_v3/` (sem `build/`,
`node_modules/`, temporários); limpar raiz (`temp_*.txt`, `backup_lines.txt`,
`novasources_*.txt`, `docs/_backup`, `prototipo` avaliado); `.gitignore`
cobrindo builds e caches; `host_check.sh` reescrito em bash portátil (zero
`pwsh.exe`); esqueleto de CI (host_check + testes nativos em Linux);
skills atualizadas para caminhos/contratos v4.

Critérios de saída: clone limpo < 100 MB (sem reference); `host_check.sh`
verde em Linux e Windows; CI executando em pushes; skills sem referências ao
layout antigo.

## Fase 1 — Esqueleto arquitetural + board/ + CI host

Entregas (port seletivo do v3 é bem-vindo — core era bom): `core/` (EventBus,
StateStore, UiDispatcher, RequestOrchestrator, ServiceManager, ActionQueue
com overflow contado), `models/` (AppState com valid/stale/source),
`utils/` (Result<T>/Status, http_client com cap-como-falha), **`board/`
(IBoard + MockBoard + WaveshareBoard mínima: display, locks, brilho)**,
interfaces de provider, ScreenRegistry vazio + ScreenSpec, `main/` só wiring.
Testes nativos de core no CI.

Critérios de saída: `idf.py build` e host_check verdes; testes de
EventBus/StateStore/Orchestrator/ActionQueue passando no CI; nenhuma
dependência CMake violando o §4 do ARCHITECTURE; app_main < 300 linhas.

## Fase 2 — Boot, display e shell de UI registrável

Entregas: display via board/ (PSRAM double-buffer, render parcial, rotação
PPA — receitas do RESOURCE-BUDGET); boot resiliente (retry + breadcrumb, sem
`abort()`); shell (rail, topbar, dots, toasts, teclado) iterando o
ScreenRegistry; telas Boot e Home registradas com view-model e máscara de
invalidação; tema e fontes portados.

Critérios de saída: boot até Home < 10 s; navegação fluida entre Home e um
placeholder registrado; falha simulada de display gera backoff+breadcrumb e
não boot loop quente; zero flag de repintura fora do shell.

## Fase 3 — Conectividade, tempo e onboarding

Entregas: transporte C6/ESP-Hosted via board-service; SetupService (NVS
versionado + migração, wizard, scan, reconexão limitada por boot);
ClockService (RTC plausível → hora imediata; NTP refina; timezone via tabela
central); tela Setup registrada.

Critérios de saída: onboarding completo em unidade zerada; reboot reconecta
sozinho; hora correta imediata com RTC válido e sem rede; teste de host
cobrindo migração de schema NVS (v desconhecida não bricka).

## Fase 4 — Dados reais, cache offline e degradação

Entregas: NetworkWorker (task única serializada); providers CoinGecko,
Open-Meteo, AwesomeAPI atrás de interfaces **com fixtures de payload real e
malformado**; CacheStore (blobs versionados, atômico, throttle 30 min);
políticas de rede do RESOURCE-BUDGET; circuit breaker ativo fim a fim.

Critérios de saída: boot offline mostra cache com stale sinalizado; queda de
API abre breaker e recupera (testado com fixture/fault injection); parsing
100% coberto por fixtures no CI; watermark de heap estável durante 24 h de
fetches (log de bancada anexado).

## Fase 5 — Telas funcionais do MVP

Entregas: Market (spot + OHLC com modo focado), Weather (previsão),
Settings (brilho c/ preview, tema, hora, volume via services/audio + IAudio,
rede, reboot), Notificações (badge, overlay, toast), tela Sistema
(diagnóstico: heap, rede, reset reason, reboots, breakers).

Critérios de saída: nenhuma tela placeholder no rail; checklist do
UI-PATTERN §7 cumprido por tela (incl. tempo de update medido); view-models
com testes host (válido/stale/ausente); app_main permanece só wiring.

## Fase 6 — Observabilidade, soak e resiliência provada

Entregas: coredump para flash + procedimento de triagem; contadores
persistidos (reboots, falhas por domínio, overflow de fila, ResourceWarning);
watermarks contínuos; roteiro de fault injection (Wi-Fi off, DNS falho, API
500, payload truncado, cache corrompido, NVS cheio).

Critérios de saída: **soak de 7 dias** sem reboot involuntário e heap estável
(relatório em docs/reports/); todos os cenários de falha do roteiro passando;
crash proposital produz coredump recuperável e triado.

## Fase 7 — OTA + release PROD seguro (fecha o MVP)

Entregas: tabela de partições A/B + otadata (definida já na Fase 1 para não
reparticionar); fluxo OTA local (upload na LAN) com verificação de assinatura,
rollback automático por health-check pós-boot; procedimento PROD (Secure Boot
v2, Flash Encryption RELEASE, NVS Encryption) ensaiado em unidade de bancada
sacrificável; `OPERATIONS.md` finalizado com runbooks.

Critérios de saída: OTA aplicada e revertida (rollback forçado) com sucesso
≥ 3 ciclos; unidade PROD provisionada e funcionando com update subsequente;
anti-rollback coerente com o fluxo; nenhuma chave no repositório.

## Fases 8–11 (planejadas, sem compromisso de desenho)

- **8 — v1.0**: modo noite, timer/Pomodoro, álbum (custo declarado no
  RESOURCE-BUDGET antes), agenda funcional, perfis.
- **9**: Sonoff LAN/Tasmota/ESPHome/MQTT, cenas, sensores locais.
- **10**: mídia, ambient intelligence.
- **11**: server opcional (descongela `shared/`), voz, NoiseBot.

Cada uma exigirá mini-PRD + ADRs próprios; nada aqui é contrato ainda.
