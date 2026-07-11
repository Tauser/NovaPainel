# NovaPanel — Estratégia de Testes (baseline v4)

> Corrige a maior fraqueza de disciplina do v3: 121 linhas de teste no total,
> zero cobertura de parsing, validação host dependente de Windows. No v4,
> teste é gate, não cortesia.

## 1. Pirâmide

```text
4. Soak / campo         7 dias contínuos, fault injection, métricas (Fase 6)
3. Placa                BSP, display, touch, rede real, glitch visual
2. Build IDF            idf.py build (gate de merge)
1. Host (rápido, CI)    core/ · models/ · view-models · services c/ mocks
                        · parsing de providers com fixtures
```

Regra de desenho para testabilidade: lógica desacoplada de ESP-IDF sempre que
possível; tudo em `core/`, `models/`, `ui/viewmodels/` e parsers de provider
compila e roda no host por construção (shims de esp_log/esp_timer/freertos).

## 2. Testes host obrigatórios (nível 1)

| Alvo | Cobertura mínima |
|---|---|
| EventBus | subscribe/unsubscribe, publish reentrante, ordem |
| StateStore | mutação→evento, snapshot, acessores granulares |
| RequestOrchestrator | intervalo, rate limit, breaker Closed→Open→HalfOpen→Closed, backoff crescente com jitter dentro da faixa |
| ActionQueue | FIFO, overflow contado e logado (nunca silencioso) |
| UiDispatcher | relevância, coalescing (N eventos do mesmo tipo → 1), preservação de eventos com payload |
| View-models (por tela) | dado válido, stale, ausente/inválido |
| Services de dados | refresh com provider fake: sucesso, falha→cache, falha→sem cache |
| SetupService (lógica) | migração de schema NVS: v atual, v legada, v futura desconhecida |

## 3. Fixtures de provider (nível 1 — inovação obrigatória do v4)

Para CADA provider, versionado em `firmware/tests/fixtures/<provider>/`:

- `ok.json` — payload real capturado da API (re-capturar quando a API mudar);
- `malformed.json` — JSON inválido;
- `missing_fields.json` — campos esperados ausentes/nulos;
- `truncated.json` — corte no meio (simula cap de buffer);
- casos específicos (ex.: OHLC com array vazio, número fora de faixa).

Teste garante: `ok` → struct correta; todos os demais → falha limpa
(sem crash, sem struct parcialmente válida com `valid=true`).

**Regra de PR:** mudança em parser sem fixture nova/atualizada = reprovado.

## 4. host_check.sh (portátil — ADR-0015)

- Bash puro + `g++ -Wall -Wextra` contra shims; roda idêntico em Linux, WSL,
  macOS e CI. Nenhuma dependência de `pwsh.exe` ou SO.
- Modos: padrão (compila componentes host-checkáveis), `--app` (syntax-check
  do main), `--tests` (compila e RODA os testes de nível 1).
- Arquivos hardware-only (WaveshareBoard, BSP) são skip declarado; seus
  headers permanecem livres de dependência para manter quem os inclui
  host-checkável.

## 5. CI (gate de merge)

Pipeline mínimo (Linux):

1. `host_check.sh --tests` (níveis 1);
2. lint de arquitetura: script que valida os `REQUIRES` do CMake contra a
   matriz de dependências do ARCHITECTURE §4 (ui/ não pode requerer services,
   etc.) — barato e mata acoplamento na origem;
3. verificação de higiene: nenhum `build/`, `node_modules/`, arquivo > 5 MB
   não justificado, segredo detectável.

Build IDF completo: obrigatório localmente antes de merge (gate manual
enquanto não houver runner com ESP-IDF; automatizar quando viável).

## 6. Testes em placa (nível 3 — roteiro por fase)

Cada fase do ROADMAP define os seus; invariantes permanentes:

- boot cronometrado (< 10 s até tela útil);
- tempo de `update()` por tela logado (< 100 ms);
- ausência de glitch visual com rede ativa (observação padronizada: 10 min
  de uso com fetches forçados);
- heap watermark logado antes/depois de cada cenário.

## 7. Fault injection (nível 3/4 — roteiro da Fase 6)

Cenários mínimos, cada um com comportamento esperado documentado:

Wi-Fi desligado no meio do uso · AP some e volta · DNS falha · API responde
500/429 · payload truncado (cap) · payload malformado · cache corrompido
(bytes aleatórios) · LittleFS cheio · NVS cheio · reboot durante escrita de
cache · hora do RTC absurda · queda de energia em uso.

Esperado sempre: UI operável, degradação sinalizada, recuperação sem
intervenção, contadores incrementados.

## 8. Soak (nível 4 — critério de saída da Fase 6)

7 dias contínuos, fetches reais, relatório em `docs/reports/` com: uptime,
reboots (zero involuntários), watermarks de heap (sem tendência de queda),
aberturas de breaker, overflows de fila. Sem relatório de soak não existe
alegação de estabilidade — em nenhum documento.
