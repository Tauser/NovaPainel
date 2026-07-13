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
| Fase atual | **Fase 4 — Dados reais, cache offline e degradação** | 2026-07-12 |
| Firmware ativo | esqueleto v4 criado em `firmware/` (Fase 1) | 2026-07-11 |
| Hardware | validado (herdado da Fase 0 do baseline v3) | 2026 |
| Build PROD validado em bancada | não | — |
| OTA operacional | não | — |
| Soak (7 dias contínuos) no tree atual | não | — |

## Fases (espelho do ROADMAP — marcar apenas aqui)

```text
Fase 0  - Preparação do repositório e tooling            [concluída em 2026-07-11]
Fase 1  - Esqueleto arquitetural + board/ + CI host      [concluída em 2026-07-11]
Fase 2  - Boot, display, shell de UI registrável         [concluída em 2026-07-11]
Fase 3  - Conectividade, tempo e onboarding              [concluída em 2026-07-12]
Fase 4  - Dados reais, cache offline e degradação        [em andamento]
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

- Nenhuma evidência de estabilidade de longa duração em nenhum tree.
- Nenhuma dívida conhecida bloqueia o encerramento da Fase 3.
- `NetworkWorker` e `CacheStore` agora têm um consumidor real: `MarketService`
  (BTC via CoinGecko, USD/BRL via AwesomeAPI). Weather/Open-Meteo ainda não
  entrou -- precisa de latitude/longitude em `UserPreferences`, que não
  existe no modelo hoje (mudança pequena, mas ainda não feita).
- Nada disso foi validado em rede real nesta sessão: `HttpClient` real só
  compila sob `ESP_PLATFORM` (não há ESP-IDF neste ambiente); o circuit
  breaker do `RequestOrchestrator` e o watermark de heap sob fetch real
  continuam sem evidência de bancada.
- `NetworkWorker`, `CacheStore`, os providers e o `MarketService` só foram
  validados por `host_check.sh --app --tests` (parsing coberto por fixtures
  reais/malformadas, sem rede) nesta sessão; `idf.py build`/bancada ainda
  não confirmaram a task subindo, a partição LittleFS montando, nem um
  fetch HTTPS de verdade no ESP32-P4.

## Evidência de encerramento da Fase 2

- `board/WaveshareBoard` agora usa buffer parcial com `double_buffer=true`,
  `sw_rotate=true` e rotação 180° via BSP, alinhado às receitas de
  `RESOURCE-BUDGET.md`.
- `ui/` agora expõe `ScreenSpec` no padrão registrável do baseline v4
  (`id/title/invalidation_mask/build/update/on_enter/on_leave`) e um
  `UiShell` que itera o `ScreenRegistry`.
- Telas `Boot`, `Home` e um placeholder de navegação foram registradas; o
  shell publica navegação via `StateStore`/`EventType::ScreenChanged`.
- `app_main` agora entra em loop de boot/UI com splash curto, transição
  automática de Boot→Home e retry de display com backoff.
- Flags operacionais de retry/breadcrumb do display agora são persistidas em
  NVS via `cache/BootBreadcrumbStore`, com dedup de escrita e limpeza após
  boot bem-sucedido.
- `UiShell` agora possui toast overlay real e um painel de teclado
  compartilhado sob posse do shell, em vez de texto estático no topbar.
- O topbar da shell deixou de usar texto fixo e agora reflete o estado atual
  da aplicação via view-model puro host-testável (`sem hora/rede`, ou relógio
  quando existir dado válido), mantendo a UI desacoplada de services.
- View-models puros de Boot/Home foram adicionados ao host-check com cobertura
  nativa.
- `sdkconfig.defaults` passou a habilitar as fontes LVGL usadas pelo shell.
- A política de retry/restart do boot virou lógica pura em `utils/`, com
  teste nativo cobrindo falha simulada de display, backoff, breadcrumb e
  decisão de restart.
- O boot validado na COM8 detecta PSRAM, conclui a inicialização do display e
  do touch, e o splash configurado (`kBootSplashMs = 1500`) coloca a Home bem
  abaixo do critério de saída de 10 s.
- A validação em hardware na COM8 passou a detectar PSRAM de 32 MB, concluir
  `Display initialized`, detectar o touch GT911 e manter o boot estável sem
  reboot durante a captura serial após reset manual.
- A atualização incremental da shell também foi reflashada na COM8 e repetiu
  o boot limpo com PSRAM, display e touch inicializados sem reboot.
- A navegação base Home ↔ Placeholder ficou exposta pelo rail do `UiShell`,
  com troca de tela via `StateStore`/`ScreenChanged`, e foi confirmada em
  bancada pelo uso do tree atual.
- A auditoria do tree não encontrou flags de repintura fora do shell:
  o fluxo de update visual passa por `UiDispatcher.pending_mask` →
  `UiShell::render()` → `ScreenSpec::update()`.
- Validações verdes no tree local após o avanço:
  `tools/scripts/host_check.sh --app --tests`,
  `tools/scripts/ci_hygiene.sh`,
  `tools/scripts/architecture_check.sh`,
  `idf.py build`.

## Evidência de encerramento da Fase 3

- O componente `services/` foi criado no baseline v4, com `ClockService`
  isolado de UI e mutando relógio apenas via `StateStore`.
- O `WaveshareBoard` agora também sobe o transporte ESP-Hosted/SDIO do
  ESP32-C6 no `bring_up()`, inicializando NVS/esp_netif/event loop, criando o
  STA default e chamando `esp_wifi_init/set_mode/start` dentro da HAL em vez
  de deixar esse wiring no `main/`.
- O baseline de configuração do firmware agora declara explicitamente
  `esp_hosted` + `esp_wifi_remote` para o host ESP32-P4 com C6 onboard,
  incluindo o perfil `P4_DEV_BOARD_FUNC_BOARD` e os buffers `WIFI_RMT_*`
  compatíveis com o exemplo oficial do componente.
- `IBoard` passou a expor leitura de RTC (`rtc_unix_time_s()`), permitindo
  que a política de hora imediata do ADR-0014 viva na HAL em vez de vazar
  para a UI ou para o `main/`.
- `ClockService` já aplica a regra de plausibilidade do ADR-0014:
  hora abaixo do limiar fica inválida/`DataSource::None`; hora plausível é
  publicada no `ClockState` sem inventar data.
- O `MockBoard` ganhou RTC injetável para teste host; o `WaveshareBoard`
  agora lê `time(nullptr)` como fonte do RTC do SoC, alinhado à referência do
  v3 e ao contrato de `HARDWARE.md`.
- `app_main` agora registra o `ClockService` no `ServiceManager`, preservando
  o `main/` como wiring da fase.
- A validação em hardware na COM8 mostrou que a unidade atual ainda sobe com
  `ClockService: no plausible RTC time yet`, então o comportamento correto do
  tree continua sendo expor estado sem hora até o futuro sync por NTP.
- O estado canônico agora inclui `preferences` e `setup`, com setters no
  `StateStore` e evento dedicado `SetupChanged`, preparando onboarding e
  migração/versionamento sem colocar lógica na UI.
- `SetupService` foi adicionado em `services/`, aplica defaults de produto
  (timezone/brilho/24h), inicializa `onboarding_required=true` e permanece
  estritamente offline, sem scan/conexão reais nesta etapa.
- A lógica de storage do setup foi separada em função pura host-testável,
  cobrindo bootstrap de schema, carga de preferências persistidas e o caso
  exigido pelo roadmap: schema futuro desconhecido é ignorado sem bricking.
- O `SetupService` agora abre NVS no namespace `setup`, lê `schema_v`,
  inicializa schema ausente e carrega `onboarding_done`, brilho, formato 24h
  e timezone quando compatíveis com a versão atual.
- O estado canônico de setup agora também carrega status explícito de Wi-Fi
  (`wifi_connect_status`, `wifi_scan_status`, redes escaneadas, tentativas de
  reconexão e flag `transport_ready`), mantendo a UI acoplada apenas ao
  `StateStore`.
- O `SetupService` agora lê `wifi_ssid`/`wifi_pass` da NVS versionada, marca
  `wifi_configured` quando há SSID persistido e tenta `esp_wifi_set_config` +
  `esp_wifi_connect` automaticamente no boot quando o transporte ESP-Hosted
  já está ativo.
- Em unidade zerada, o `SetupService` agora dispara scan automático quando o
  transporte sobe; o scan usa `esp_wifi_scan_start`, de-duplica SSIDs, ordena
  por RSSI e degrada para `WifiScanStatus::Failed` com timeout/retry em vez de
  travar o fluxo.
- Em unidade com credenciais salvas, a reconexão agora é limitada por boot
  (`kMaxWifiRetries = 5`, delay de 3 s, timeout de 20 s por tentativa) e
  termina em `WifiConnectStatus::Failed` quando esgota o orçamento desta
  inicialização.
- A tela `Setup` deixou de ser apenas placeholder textual e agora expõe um
  wizard mínimo orientado a estado (`Wifi` → `TimezoneAndFormat` →
  `Confirmation`), com botões que publicam intenção via `ActionQueue` e são
  drenados pelo `main_loop` até o `SetupService`.
- O passo de Wi-Fi agora renderiza a lista real de redes escaneadas no próprio
  `SetupScreen`, mantendo a seleção em UI local e publicando apenas a intenção
  de scan/submissão via `ActionQueue`; por segurança, o avanço fica liberado
  apenas para redes abertas enquanto a UX de senha ainda não existe.
- O shell agora possui um manager mínimo do teclado compartilhado em `ui/`,
  com painel `lv_keyboard` real e ajuste de layout do conteúdo/dots enquanto o
  teclado está aberto, sem mover lógica de onboarding para dentro do shell.
- O passo de Wi-Fi do wizard agora também cobre redes protegidas: a
  `SetupScreen` anexa um `lv_textarea` de senha ao teclado compartilhado do
  shell, mantém o rascunho local da senha na própria tela e só libera o avanço
  quando há SSID selecionado e senha preenchida para rede protegida.
- O passo de timezone/formato agora usa uma lista curada central de fusos do
  produto (`America/Sao_Paulo`, `America/Manaus`, `America/Rio_Branco`,
  `America/Noronha`, `UTC`) e um toggle explícito de 12h/24h antes da etapa de
  confirmação.
- O wizard foi validado em bancada pelo usuário em unidade zerada: após flash
  e limpeza de NVS, o setup inicia uma vez, lista redes, aceita rede protegida
  com senha, avança fuso/formato, confirma, entra na Home e não reaparece no
  reboot seguinte.
- A reconexão automática pós-setup também foi validada em bancada pelo
  usuário: após concluir o onboarding e reiniciar, o firmware permanece fora
  da tela Setup e reconecta usando as credenciais salvas.
- O `ClockService` agora inicia SNTP somente depois de o Wi-Fi chegar a
  `WifiConnectStatus::Connected`, preservando a publicação imediata de RTC
  plausível e refinando a hora via `pool.ntp.org` sem bloquear o fluxo offline.
- A validação final em bancada foi executada pelo usuário após novo flash em
  2026-07-12: RTC/NTP/timezone passaram no hardware, incluindo hora correta na
  topbar com o timezone escolhido no wizard.
- O build do alvo voltou a passar com o transporte ESP-Hosted habilitado no
  tree (`idf.py build`), confirmando o baseline de dependências/`sdkconfig`
  necessário para o host ESP32-P4 conversar com o C6 via SDIO.
- A regressão visual de tela branca antes do boot foi tratada seguindo a
  referência v3: `WaveshareBoard` mantém o backlight apagado após iniciar o
  BSP e o `main/` só aplica brilho depois do primeiro render da shell; o
  firmware foi reflashado na COM8, aguardando confirmação visual em bancada.
- O log de bancada da COM8 mostrou que o ESP-Hosted/C6 pode bloquear por cerca
  de 21 s tentando subir o transporte antes de falhar em
  `esp_hosted_get_coprocessor_fwversion`; por isso o contrato da HAL foi
  separado: `bring_up()` volta a cobrir o boot mínimo de display e
  `start_network_transport()` roda só depois do primeiro frame, mantendo a
  falha do C6 como degradação de rede em vez de atraso de boot visual.
- A inicialização do transporte voltou a seguir a receita validada do v3:
  `WaveshareBoard` não chama mais `esp_hosted_get_coprocessor_fwversion()` nem
  força `esp_hosted_connect_to_slave()` no caminho do produto; o bring-up da
  rede passa por `esp_wifi_remote` (`esp_wifi_init/set_mode/start`), evitando
  que uma consulta diagnóstica do coprocessador gere erro ruidoso quando o link
  ESP-Hosted ainda não está up.
- O erro de bancada `net80211: OS adapter function version error` expôs uma
  configuração inválida: `CONFIG_ESP_HOST_WIFI_ENABLED` estava ligado junto de
  `esp_wifi_remote`, apesar do Kconfig declarar os modos como mutuamente
  exclusivos. O default agora deixa `ESP_HOST_WIFI` desligado, permitindo que o
  header injetado do `esp_wifi_remote` forneça o `wifi_init_config_t` esperado
  pelo ESP-Hosted/C6.
- `SetupService` agora evita spam de `esp_wifi_scan_start` quando
  `transport_ready=false`: a solicitação de scan degrada para
  `WifiScanStatus::Failed` sem agendar retry até o transporte estar realmente
  disponível.
- A tela `Setup` foi registrada no `ScreenRegistry` com view-model e
  placeholder explícito da Fase 3, sem acoplar request ou persistência à UI.
- Revisão pós-bring-up do C6 corrigiu 10 problemas encontrados por análise
  multi-agente do diff da Fase 3:
  - `SetupService` chamava `esp_wifi_scan_start/set_config/connect` e
    registrava handlers de evento direto, furando a regra "hardware só atrás
    de `IBoard`". Todo o driver Wi-Fi (scan, connect, eventos) migrou para
    `WaveshareBoard`/`MockBoard` atrás de `IBoard::wifi_*`; `SetupService`
    agora só consome esse polling.
  - O passo de Wi-Fi do wizard chamava `begin_wifi_connect()` sem checar
    `transport_ready`, e uma falha de config desligava o auto-reconnect pelo
    resto do boot. `sync_transport_state()` agora dispara a conexão quando o
    transporte fica pronto, e falhas entram no mesmo backoff usado para
    reconexão.
  - `board.start_network_transport()` bloqueava o loop principal (até ~21s
    esperando o C6) antes de `action_queue`/`service_manager` começarem a
    rodar, deixando o touch mudo nesse intervalo. Virou
    `start_network_transport_async()`, rodando em task própria dentro de
    `WaveshareBoard`.
  - O relógio da topbar ignorava o timezone escolhido no wizard (sempre UTC).
    `timezone_catalog` ganhou offset em minutos e `shell_chrome_view_model`
    aplica o offset de `preferences.timezone`.
  - `system.source` ficava hardcoded em `Mock` mesmo em hardware real;
    `display_breadcrumb`/`display_retry_count` do boot recovery eram
    recalculados a partir de dados obsoletos e o toast de retry nunca
    disparava; o brilho inicial usava o default antes do `SetupService`
    carregar o valor salvo em NVS; `ClockState.last_update_ms` usava epoch
    (segundos RTC) enquanto o resto do código trata esse campo como
    monotônico. Todos corrigidos em `app_main.cpp`/`clock_service.cpp`.
  - Navegação "Voltar" do wizard estava hardcoded em `main/app_main.cpp`
    (violando "main/ é wiring"); virou `SetupService::go_back()`.
- Validações verdes deste avanço:
  `tools/scripts/host_check.sh --app --tests`,
  `tools/scripts/architecture_check.sh`,
  `tools/scripts/ci_hygiene.sh`.
  `idf.py build` ainda não foi confirmado nesta sessão (sem toolchain
  ESP-IDF disponível aqui) — pendente de validação em bancada antes do
  próximo flash.

## Evidência parcial da Fase 4

- `services/NetworkWorker` entrou no tree como a task única de execução
  HTTP prevista no ADR-0004: services de dados registram
  `{RequestDomain, RefreshFn, contexto}` (ponteiro de função + contexto, sem
  `std::function`) em vez de subirem task própria.
- A escolha de qual domínio "devido" rodar (`select_fetcher_index`) e o
  gate de rede (`network_available`, baseado em
  `SetupState.wifi_connect_status == Connected`) são funções puras
  host-testáveis, separadas do loop da task real (`run()`, só compilado sob
  `ESP_PLATFORM`) — cobertas por teste nativo incluindo empate de
  prioridade por ordem de registro e prioridade `Critical` vencendo
  `Normal`.
- `RequestOrchestrator` ganhou `priority_for()` para o worker consultar a
  prioridade configurada por domínio sem duplicar a política.
- Ownership resolvido conforme a Seção 6 do `ARCHITECTURE.md`: a partir de
  agora só a task do `NetworkWorker` chama `RequestOrchestrator::tick()` /
  `mark_started()` / `mark_finished()`; a chamada que o `main_loop` fazia a
  cada 50 ms foi removida para não haver acesso concorrente sem lock ao
  mesmo objeto (o `RequestOrchestrator` não é thread-safe, ao contrário do
  `StateStore`).
- `NetworkWorker` já é instanciado e iniciado em `app_main`, mas sem
  nenhum fetcher registrado ainda (providers reais entram em seguida);
  a task sobe e fica ociosa aguardando `wifi_connect_status == Connected`.
- Validações verdes deste avanço: `tools/scripts/host_check.sh --app
  --tests`, `tools/scripts/architecture_check.sh`,
  `tools/scripts/ci_hygiene.sh`. `idf.py build` e validação em bancada
  (task realmente sobe no ESP32-P4, stack de 8 K words não estoura) ainda
  não foram feitos nesta sessão — sem toolchain ESP-IDF disponível aqui.
- `cache/CacheStore` entrou no tree como a persistência LittleFS prevista
  no ARCHITECTURE.md (blobs versionados, escrita atômica via tmp+rename).
  Blobs são bytes opacos (`save`/`load` recebem ponteiro+tamanho) para
  manter `cache/` desacoplado de `models/`; quem serializa é o chamador
  em `services/`, que ainda não existe (nenhum service grava blob nesta
  sessão).
- O throttle de no máximo 1 escrita a cada 30 min por domínio
  (RESOURCE-BUDGET.md §4: erase de flash compete com o refresh do
  display) vive dentro do `CacheStore`, não no chamador — mesma regra de
  "invariante mora em UM lugar" já usada no `WaveshareBoard` para o lock
  de I2C compartilhado. Escrita fora da janela retorna `RateLimited` sem
  tocar a flash; a janela é por sessão (reinicia a cada boot), não
  persistida em NVS.
- `load()` rejeita blob com header incompatível (magic/versão/tamanho
  esperado) em vez de interpretar bytes às cegas — mesma postura do
  `SetupStorageLogic` para schema desconhecido.
- Depende do componente gerenciado `joltwallet/littlefs` (herdado do
  `reference/firmware_v3`, já provado em campo) e da partição `storage`
  (subtype `spiffs`, 10 MB) já reservada em `partitions.csv` desde a
  Fase 1.
- `CacheStore` é montado em `app_main` (`cache_store.mount()`) só para
  provar que a partição sobe; nenhum service grava/lê ainda.
- Validado com `tools/scripts/host_check.sh --app --tests`,
  `tools/scripts/architecture_check.sh`, `tools/scripts/ci_hygiene.sh`
  (mount/save/load simulados em memória no host, sem LittleFS real).
  `idf.py build` e validação em bancada (partição `storage` monta de
  fato, escrita sobrevive a reboot) ainda não foram feitos nesta sessão
  — sem toolchain ESP-IDF disponível aqui.
- `utils/json_value.hpp` traz um parser JSON próprio e mínimo (objeto,
  array, número, string, bool, null; limite de profundidade 32 contra
  stack overflow em payload malicioso/aninhado demais). Existe porque nem
  o host (`host_check.sh`) nem este ambiente de sessão têm `cJSON`
  disponível sem um `idf.py build` real, e o ADR-0007 exige parsing
  coberto por fixtures no CI, não só no alvo -- decisão tomada com o
  usuário (alternativas descartadas: vendorizar `cJSON` sem poder validar
  o arquivo real, ou deixar o parsing sem cobertura de host).
- `utils/http_client.hpp` deixou de ser stub: `HttpClient::get()` agora
  usa `esp_http_client` de verdade sob `ESP_PLATFORM`, com mutex próprio
  serializando todo GET do firmware (RESOURCE-BUDGET.md §1 regra 1 --
  além da serialização de política que o `NetworkWorker` já fazia, esta é
  a garantia estrutural citada no documento), corpo alocado em SRAM
  interna (`MALLOC_CAP_INTERNAL`) com o cap de 48 KB já existente, e
  `esp_crt_bundle_attach` para validação TLS. Resposta maior que o cap
  vira falha (`Truncated`), nunca truncamento silencioso. No host
  continua um stub (`Unavailable`) -- não há stack de rede para simular
  com segurança num teste unitário.
- Dois providers reais entraram em `providers/`: `CoinGeckoProvider`
  (BTC spot) e `AwesomeApiProvider` (USD/BRL). Cada um separa parsing
  puro (`parse_market_payload`/`parse_forex_payload`, sem IO, testado com
  fixtures reais e malformadas) do `fetch_*()` que efetivamente chama
  `HttpClient` -- mesma técnica de isolar lógica pura de IO já usada em
  `BootRecoveryPolicy`/`SetupStorageLogic`/`NetworkWorker`.
- `AwesomeApiProvider` trata campos numéricos vindos como string (formato
  real da AwesomeAPI, ex. `"bid": "5.4321"`), com fallback para número
  JSON puro por robustez a mudança de schema.
- `services/MarketService` virou o dono do domínio `market` do
  `StateStore`: funde as duas fontes (BTC e USD/BRL) num único
  `MarketState` sem uma zerar o campo da outra, já que o modelo atual
  trata os dois como um domínio de dado só (`AppState.market` não separa
  cripto de câmbio -- possível candidato a ADR futuro se isso continuar
  incomodando quando o Market real da Fase 5 chegar).
- `MarketService::load_from_cache()` repõe o `StateStore` a partir do
  `CacheStore` antes do primeiro fetch de rede (`stale=true`,
  `source=Cache`), cobrindo o critério de saída "boot offline mostra
  cache com stale sinalizado" -- validado por teste nativo (grava via
  `refresh_crypto`/`refresh_forex` simulado, contrói um `MarketService`
  novo por cima do mesmo `CacheStore` e confere que o boot restaura os
  dois valores com o source certo).
- Falha de fetch (provider retornando erro) preserva o último valor bom
  no `StateStore` em vez de zerar -- só o retorno `false` para o
  `NetworkWorker`/`RequestOrchestrator` contam a falha para o circuit
  breaker.
- `CoinGeckoProvider`/`AwesomeApiProvider`/`MarketService` são
  instanciados e conectados em `app_main`: `market_service.load_from_cache()`
  roda logo após `cache_store.mount()` (antes de qualquer fetcher
  registrado, para o primeiro frame já poder mostrar cache stale), depois
  os dois fetchers são registrados no `NetworkWorker`
  (`RequestDomain::MarketSpot`/`RequestDomain::Forex`) antes de
  `network_worker.start()` (contrato do `NetworkWorker`: registrar antes
  de iniciar).
- Weather/Open-Meteo **não** entrou nesta sessão: o provider da v3 depende
  de latitude/longitude, que não existe em `UserPreferences` hoje --
  fica para uma próxima entrega, junto com a decisão de como esse campo
  chega no onboarding.
- Validado com `tools/scripts/host_check.sh --app --tests`,
  `tools/scripts/architecture_check.sh`, `tools/scripts/ci_hygiene.sh`.
  Nenhum fetch HTTPS real, nenhuma medição de heap sob carga e nenhuma
  abertura real do circuit breaker foram exercitados -- só o host permite
  isso hoje (parsing/merge/cache testados com fixtures e providers fake,
  rede real não). `idf.py build` e bancada continuam pendentes nesta
  sessão -- sem toolchain ESP-IDF disponível aqui.
