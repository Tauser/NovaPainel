# NovaPanel — Decisões de Arquitetura (ADRs, baseline v4)

Formato curto: contexto → decisão → motivo/consequências. Numeração reinicia
no v4; os 35 ADRs do baseline v3 permanecem no histórico git (arquivo anterior
a 2026-07-02) e são citados como `[v3 ADR-00xx]` quando uma decisão é herdada.

---

## ADR-0001 — Reconstrução total (baseline v4)

**Contexto:** análise arquitetural independente (2026-07-02, ver
`ANALISE-ARQUITETURAL-2026-07-02.md`) constatou: núcleo bom, mas drift
documental grave (CLAUDE.md descrevendo firmware inexistente), HAL abandonada,
UI singleton com invalidação N×M no main, providers sem interface, teste
raso, sem OTA. Decisão do dono do projeto: recomeçar docs e firmware.
**Decisão:** baseline v4. Documentação recriada do zero como fonte única;
firmware novo em `firmware/`; código v3 vira `reference/firmware_v3/` para
port seletivo. Premissas provadas em campo ficam fixas (hardware, ESP-IDF +
LVGL, offline-first, StateStore/EventBus, rede serializada, mitigação de
barramento).
**Consequências:** custo de reimplementação aceito conscientemente; em troca,
os contratos estruturais (board/, ScreenSpec, interfaces, ownership) nascem
na Fase 1 em vez de "depois". Port seletivo é permitido e incentivado onde o
v3 era bom (core, orquestrador, cache, receitas de BSP).

## ADR-0002 — STATUS.md como fonte única de estado

**Contexto:** a causa-raiz da degradação do v3 foi documentação afirmando
estados divergentes (duas numerações de fase, CLAUDE.md desatualizado) num
repo operado por agentes.
**Decisão:** somente `docs/STATUS.md` afirma estado. Demais docs descrevem
alvo/política. Fechamento de fase atualiza STATUS no mesmo commit
(Definition of Done). Divergência é tratada como bug com prioridade de crash.

## ADR-0003 — Monorepo, offline-first e server opcional [herda v3 ADR-0001/0002]

**Decisão:** monorepo (`firmware/`, `docs/`, `shared/`, `tools/`, `skills/`,
`reference/`); o firmware nunca depende de `server/`. `shared/` fica
**congelado** (sem contratos novos) até existir a segunda ponta (Fase 11).

## ADR-0004 — Rede: orquestração central + execução serializada [herda v3 ADR-0004/0012/0029/0035]

**Contexto:** validado em campo no v3: 3 TLS simultâneos esgotavam a SRAM
interna (~130 KB/handshake, heap a 173 KB) e derrubavam os serviços; a
solução em duas fases (mutex global de HTTP + NetworkWorker único por
prioridade) resolveu.
**Decisão:** RequestOrchestrator (política: intervalo, rate limit, circuit
breaker Closed→Open→HalfOpen, backoff exponencial com jitter) separado do
NetworkWorker (execução: 1 HTTPS por vez, gap de 400 ms, prioridade). Services
de dados não têm task própria. `CONFIG_MBEDTLS_DYNAMIC_BUFFER=y`. Keep-alive
descartado nos intervalos atuais (servidor fecha idle antes do próximo fetch).

## ADR-0005 — board/ obrigatória com locks semânticos

**Contexto:** o v3 aboliu a HAL; o BSP e ~300 linhas de driver de áudio
vazaram para o app_main, e o invariante "display lock = mutex do I2C
compartilhado (GT911+ES8311)" ficou em comentários.
**Decisão:** `IBoard` (+ `MockBoard`, `WaveshareBoard`) desde a Fase 1.
`lock_ui()` e `lock_shared_i2c()` são nomes semânticos; a coincidência física
dos dois locks é detalhe interno da WaveshareBoard. Áudio é módulo
(`services/audio` sobre `IAudio`), nunca inline no main.
**Consequências:** host-check e testes de service sem hardware; o main volta
a ser wiring; o custo é uma camada a mais de indireção — aceito.

## ADR-0006 — UI por registro de ScreenSpec com máscara de invalidação

**Contexto:** no v3, cada evento→tela era um if/else com flags booleanas no
app_main (mapa N×M), e telas repintavam por inteiro a cada evento.
**Decisão:** telas se registram (`ScreenSpec{id, build, update,
invalidation_mask, on_enter/leave}`); o shell itera o registro; o
UiDispatcher é o único dono de coalescing; view-model puro por tela.
Detalhes: `UI-PATTERN.md` (documento normativo).
**Consequências:** tela nova sem tocar main/shell; testes host de view-model;
custo de padronizar as primeiras telas — pago na Fase 2.

## ADR-0007 — Providers atrás de interface, com fixtures obrigatórias

**Contexto:** o v3 prometia ports-and-adapters mas services dependiam de
providers concretos; zero teste de parsing — o ponto onde payload externo
muda sem aviso.
**Decisão:** `IMarketProvider`, `IWeatherProvider`, `IForexProvider` desde o
primeiro commit de cada domínio; service recebe a interface por injeção no
main. Todo provider tem fixtures versionadas (payload real capturado +
variantes malformadas/truncadas) rodando no CI.

## ADR-0008 — Modelo de concorrência com propriedade declarada

**Contexto:** o v3 sincronizava por convenção; havia data race real com
comentário afirmando segurança (`request_ohlc_period`), e fila de ações de
UI com profundidade 4 e drop silencioso.
**Decisão:** tabela de tasks e ownership no ARCHITECTURE §6 é normativa.
Campo compartilhado = atomic, mutex ou ActionQueue — declarado no header.
ActionQueue ≥ 16 com overflow logado e contado. EventBus síncrono mantido
(dados pelo StateStore, evento como sinal) [herda v3 ADR-0007/0013/0019].

## ADR-0009 — RESOURCE-BUDGET como contrato de plataforma

**Contexto:** as mitigações físicas do v3 (SRAM para HTTP/JSON, render
parcial, throttle de flash, 1 TLS) estavam corretas mas espalhadas em
comentários de 4 arquivos.
**Decisão:** `RESOURCE-BUDGET.md` é normativo: regras de alocação por tipo de
dado, limiares de heap com ação (`ResourceWarning` com handler real),
orçamento de escrita de flash e de rede. Feature nova declara custo antes de
entrar no roadmap. Truncamento de corpo HTTP passa a ser falha de request.

## ADR-0010 — OTA A/B antes de qualquer unidade PROD lacrada

**Contexto:** o v3 tinha partição factory-only e, simultaneamente,
`sdkconfig.prod` com Flash Encryption RELEASE (irreversível) e anti-rollback
— combinação que produziria unidade lacrada sem caminho de atualização.
**Decisão:** a tabela de partições nasce com `ota_0/ota_1/otadata` na Fase 1
(evita reparticionar depois). Fluxo OTA local com assinatura + rollback
automático por health-check é pré-requisito do provisionamento PROD (Fase 7).
Anti-rollback só é ativado junto do fluxo OTA.

## ADR-0011 — Produto pessoal: pt-BR e Brasil como default consciente

**Contexto:** o v3 hardcodeava strings pt-BR nas telas, tabela de 5 timezones
brasileiros e localização default Brasília — implícito e espalhado.
**Decisão:** o MVP é single-locale (pt-BR/Brasil) por escolha explícita, mas:
strings em tabela única (`strings_ptbr.hpp`), timezone/localização em
configuração central e UserPreferences. i18n completa NÃO é requisito; barato
de adotar depois se o produto deixar de ser pessoal.

## ADR-0012 — Observabilidade e coredump em PROD

**Contexto:** contradição no v3: a triagem de campo dependia de coredump, mas
o perfil PROD desligava coredump-to-flash.
**Decisão:** coredump para partição flash fica LIGADO em PROD; a partição é
apagada no provisionamento; logs em nível WARN. Contadores persistidos
mínimos: reboots, falhas por domínio, aberturas de breaker, overflows de fila,
ResourceWarnings. Watermarks de heap amostrados continuamente e expostos na
tela de sistema. [estende v3 ADR-0014/0028]

## ADR-0013 — Persistência: NVS versionada + cache LittleFS atômico [herda v3 ADR-0027/0034]

**Decisão:** NVS com `schema_v`, migração explícita, versão futura → ignora
sem brickar, dedup de escrita. Cache em LittleFS com header
magic/versão/tamanho, escrita tmp+rename, mismatch → descarte; persistência
throttlada (30 min/domínio). Segredos só em NVS (cifrada em PROD); nunca em
log. [herda v3 ADR-0008]

## ADR-0014 — Clock híbrido RTC↔NTP [herda v3 ADR-0009/0021/0032]

**Decisão:** RTC com bateria é a fonte de boot: hora plausível
(epoch > limiar) aparece imediatamente sem rede; NTP refina quando disponível;
sem hora plausível, UI mostra estado não-sincronizado — nunca inventa data.
NTP é pré-requisito funcional de HTTPS (validação de certificado).

## ADR-0015 — Validação host portátil como gate de CI

**Contexto:** o `host_check.sh` do v3 dependia de `pwsh.exe` (só Windows),
quebrando a promessa de validação portátil e impedindo CI Linux.
**Decisão:** host_check em bash puro + g++ com shims; roda idêntico em Linux,
WSL e CI. Testes nativos (core + view-models + fixtures de provider) são gate
de merge. Nenhum script de validação pode depender de SO específico.

## ADR-0016 — Tabela de partições A/B fixada na Fase 1

**Contexto:** ADR-0010 exige OTA A/B desde o início e `HARDWARE.md` determina
que as dimensões exatas sejam fixadas na Fase 1 para evitar reparticionar
unidades em campo.
**Decisão:** `firmware/partitions.csv` nasce com `nvs`, `nvs_keys`,
`phy_init`, `otadata`, dois slots OTA de 8 MB, `storage` LittleFS de 10 MB,
`coredump` de 1 MB e uma partição reservada `c6_ota` de 1 MB para o fluxo
futuro de atualização do ESP32-C6 via ESP-Hosted.
**Motivo:** 8 MB por app dá margem para LVGL, BSP e assets iniciais sem
consumir todo o flash de 32 MB; 10 MB de cache sustenta uso offline sem
pressionar os slots OTA; reservar `c6_ota` agora elimina migração destrutiva
quando o slave-OTA entrar na Fase 7.
