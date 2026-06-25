# NovaPainel - Decisões de Arquitetura (ADRs)

Formato curto. Cada ADR registra uma decisão e o motivo.

## ADR-0001 - Monorepo
**Decisão:** usar monorepo com `firmware/`, `server/`, `shared/`, `docs/` e
`tools/`.
**Motivo:** permitir evolução futura com servidor opcional sem reorganizar o
projeto, mantendo contratos compartilhados em `shared/`.

## ADR-0002 - Offline-first
**Decisão:** o firmware deve funcionar sem server.
**Motivo:** o painel precisa continuar útil mesmo sem internet ou serviço
externo. O server é opcional e futuro.

## ADR-0003 - Hardware Abstraction
**Decisão:** usar uma camada `board/` (HAL, `IBoard`) e abstrações de hardware.
**Motivo:** permitir trocar Waveshare/Elecrow/Makerfabs - ou usar `MockBoard` -
sem reescrever o core.

## ADR-0004 - RequestOrchestrator desde o início
**Decisão:** controlar requests por tela, prioridade, frequência e custo, com um
orquestrador central pelo qual todos os serviços passam.
**Motivo:** evitar consumo excessivo de CPU, rede, memória e cotas de API.

## ADR-0005 - Device Control Deferred
**Decisão:** controle de TV Samsung sai do projeto; Sonoff e automação física
ficam para roadmap futuro.
**Motivo:** reduzir risco técnico do MVP.

## ADR-0006 - Market Data Strategy
**Decisão:** usar CoinGecko REST no MVP, com intervalo de 60s e budget de até 6
chamadas/min, cache obrigatório e busca em lote. WebSocket de exchange fica para
o futuro.
**Motivo:** já funciona bem em projeto existente do usuário e atende ao MVP sem
estourar rate limit. (Operacional: registrar a Demo key gratuita do CoinGecko.)

## ADR-0007 - UI Marshaling via UiDispatcher
**Decisão:** toda atualização de UI passa pelo `UiDispatcher` e é executada na
`lvgl_task`. Nenhuma outra task toca objetos LVGL.
**Motivo:** LVGL não é thread-safe; acesso de múltiplas tasks causa travamentos.

## ADR-0008 - Secrets Storage
**Decisão:** NVS normal em desenvolvimento; avaliar NVS encryption / Flash
encryption (e Secure Boot) para release.
**Motivo:** proteger Wi-Fi, API keys e tokens em produto real.

## ADR-0009 - RTC / Offline Clock
**Decisão:** a promessa de relógio offline depende da validação de RTC/bateria na
Fase 0. Sem RTC, após reboot sem internet o horário é exibido como não
sincronizado até o NTP.
**Motivo:** não prometer uma garantia que o hardware pode não oferecer.

## ADR-0010 - ESP32-C6 Firmware / Update Strategy
**Decisão:** validar o firmware do ESP32-C6 (ESP-Hosted) e o link SDIO na Fase 0,
antes de qualquer feature pesada; definir a partição e o processo de atualização
do C6.
**Motivo:** a conectividade do P4 depende inteiramente do C6.

## ADR-0011 - Segurança projetada desde o layout (estende ADR-0008)
**Decisão:** o layout de partição (`firmware/partitions.csv`) deve ser definido já
assumindo **Flash Encryption + Secure Boot v2 (com anti-rollback)** e **NVS
Encryption nativa do ESP-IDF**, mesmo que o build de desenvolvimento mantenha
essas proteções desativadas. Em produção, segredos (Wi-Fi, API keys, tokens) ficam
em NVS cifrada; builds PROD habilitam Secure Boot v2 e Flash Encryption. Não
implementar criptografia própria (ex.: AES-GCM manual) para segredos — usar os
mecanismos da plataforma. Proteção de segredo em logs/coredump é disciplina
separada: nunca logar segredo e cifrar/desabilitar coredump em PROD.
**Motivo:** habilitar Flash Encryption e Secure Boot depois muda o layout de
partição e a gestão de chaves (eFuses são mão única), forçando retrabalho. Cripto
própria é mais arriscada e redundante frente à NVS Encryption nativa. Cifrar NVS
não impede vazamento por log — são problemas distintos. Gatilho de reavaliação do
modelo de ameaça: quando entrar automação/controle físico (a superfície deixa de
ser somente leitura).

## ADR-0012 - Resiliência de rede: circuit breaker e backpressure no Orchestrator
**Decisão:** o `RequestOrchestrator` (ADR-0004) ganha, por domínio, um **circuit
breaker** (closed/open/half-open) com **backoff exponencial + jitter** e
**backpressure** (profundidade de fila limitada, política drop-oldest). Retry é
**finito** (nunca infinito); ao abrir o circuito, o domínio degrada para cache e
tenta reconectar em segundo plano sem bloquear a UI.
**Motivo:** um provider lento ou fora do ar não pode derrubar o sistema nem
estourar CPU/cota. Mantém o offline-first concreto (ADR-0002): dado stale exibido
em vez de travar. Implementado dentro do orquestrador que já é a porta única de
saída, sem nova camada.

## ADR-0013 - Modelo de tasks e fila assíncrona para a UI
**Decisão:** o caminho de atualização de UI (`EventBus` → `UiDispatcher` →
`lvgl_task`) usa **fila FreeRTOS assíncrona com coalescing**, drenada apenas pela
`lvgl_task`. Um service lento nunca bloqueia o render. Eventos internos
service→service simples podem permanecer síncronos; a regra inviolável é "render
nunca bloqueado por service". Pinagem de cores explícita no P4 dual-core
(`lvgl_task` em um core; rede/serviços no outro), com stack size e watchdog por
task documentados. Não se promete "zero malloc": framebuffers e buffers grandes
conhecidos são pré-alocados no boot em PSRAM via `heap_caps_malloc(MALLOC_CAP_SPIRAM)`
e o LVGL recebe um pool fixo; dirty rectangles usam a API **nativa** do LVGL.
**Motivo:** LVGL não é thread-safe (ADR-0007) e EventBus síncrono na task do
publisher convida priority inversion e travamento da UI. Pré-alocar buffers
grandes evita fragmentação sob carga contínua, mas o LVGL faz malloc interno —
"zero malloc" seria irreal. Reusar o dirty-tracking nativo evita reinventar e
mantém o código testável.

## ADR-0014 - Observabilidade de campo (coredump + diagnóstico)
**Decisão:** reservar **partição de coredump** e persistir crash dumps; a tela de
sistema/status expõe motivo do último reset, contador de reboots e idade do último
dado por domínio (stale indicator). Builds PROD reduzem logs a warnings críticos
acionáveis; debug verboso fica fora de PROD.
**Motivo:** sem crash dump persistido e contexto de reset, depurar uma falha na
casa do usuário é às cegas. Indicar dado stale é parte do contrato de UX degradada
do offline-first, não enfeite.

## ADR-0015 - Integridade do cache offline e filesystem
**Decisão:** o cache local usa **LittleFS** (wear leveling), com **escrita atômica**
(grava em temporário e renomeia) e **versionamento de schema** do cache e da NVS,
com migração no boot. Não usar SPIFFS para dados que precisam sobreviver a queda de
energia.
**Motivo:** o offline-first depende do cache sobreviver a corte de energia e a
upgrades de firmware. SPIFFS não tem wear leveling robusto; sem escrita atômica e
versionamento, um OTA que muda um struct corrompe dado antigo e pode brickar a
configuração.

## ADR-0016 - WaveshareBoard real (Fase 3) e semântica de network_ready
**Decisão:** `firmware/components/board/` ganha `WaveshareBoard : IBoard`,
implementação real usando o BSP oficial `waveshare/esp32_p4_wifi6_touch_lcd_7b`
(EK79007 + GT911, validados em Fase 0). Fica no mesmo componente que
`MockBoard` (mesmo padrão de `providers/`), mas o header
(`waveshare_board.hpp`) não inclui nenhum header real de ESP-IDF/BSP — só a
implementação (`.cpp`) faz isso. Isso mantém `app_main.cpp` host-checkável
mesmo usando o board real; `tools/scripts/host_check.sh` pula explicitamente
`waveshare_board.cpp` (hardware-only, sem shim).
`BoardStatus::network_ready` significa **link ESP-Hosted/SDIO com o C6 ativo**,
não associação Wi-Fi a um AP real. `WaveshareBoard::bring_up()` chama
`esp_wifi_init/set_mode/start()` (que já sobe o transporte SDIO, confirmado em
Fase 0 Gate 9) mas nunca define SSID/senha nem chama `esp_wifi_connect()`.
**Motivo:** conectar a uma rede real com credenciais é decisão de serviço
(Fase 5, `MarketService`/Wi-Fi), não de bring-up de hardware. Separar os dois
mantém a Fase 3 escopada (board real != rede real) e evita acoplar o `IBoard`
a configuração de usuário. A separação header/implementação evita que todo
código real de hardware acabe "contaminando" `app_main.cpp` e quebrando o
host check, que é o jeito rápido do time validar lógica de wiring sem
ESP-IDF instalado.

## ADR-0017 - Tela de boot e wizard de onboarding inicial
**Decisão:**
- **Boot/splash:** não precisa de decisão nova. `AppState::current_screen` já
  nasce em `ScreenId::Boot` (era só não-renderizado até agora). Na Fase 4,
  basta adicionar um screen builder para `Boot` igual ao de `Home` - nenhuma
  mudança estrutural.
- **Wizard de onboarding inicial (multi-step):** roda no primeiro boot (sem
  config salva) e cobre, no mínimo, os passos hoje conhecidos -
  **nome de exibição** ("como quer ser chamado"), **provisionamento de
  Wi-Fi**, **fuso horário**, **formato de hora** (12h/24h) e **tema**
  (claro/escuro/automático) - extensível a mais passos depois sem mudar a
  regra abaixo.
  - A tela **nunca** persiste nem chama hardware/rede direto, em nenhum
    passo. Fluxo obrigatório: tela publica uma intenção no `EventBus` (ex.:
    `EventType::OnboardingStepSubmitted` ou eventos dedicados por passo,
    como `WifiCredentialsSubmitted`/`DisplayNameSubmitted`/
    `PreferencesSubmitted`) → um `SetupService` (novo) é o único a
    persistir em NVS e, no passo de Wi-Fi, chamar
    `esp_wifi_set_config`/`esp_wifi_connect` → o resultado
    (sucesso/falha/IP, preferências salvas) volta via `StateStore` e a tela
    só lê, nunca bloqueia esperando resposta.
  - Passos não-Wi-Fi (nome, fuso, formato de hora, tema, e quaisquer outros
    que entrarem depois) são só leitura/escrita de NVS via `SetupService` -
    mais simples que o passo de Wi-Fi, mas seguem a mesma regra de UI sem
    request direto (ADR arquitetural já vigente: UI não faz request direto,
    só `StateStore`/`EventBus`). Modelo de dados (ex.: um `UserPreferences`
    em `models/`, com `display_name`, `timezone`, `time_format_24h`,
    `theme`) é detalhe de implementação da Fase 4/5, não desta ADR -
    `ClockService` passa a ler fuso/formato dali em vez de hardcoded.
  - O mesmo `SetupService` (ou os mesmos eventos) deve ser reusável pela
    futura tela de Configurações (`screen_config.c` na referência de
    design), não só pelo wizard de primeiro boot - editar Wi-Fi ou nome
    depois não é um fluxo separado.
- **Armazenamento:** NVS, conforme ADR-0011 (sem cripto própria; builds PROD
  habilitam NVS Encryption nativa; dev pode deixar desativado). Vale tanto
  para a credencial de Wi-Fi quanto para o nome de exibição e qualquer outro
  dado de preferência que o wizard vier a coletar.
**Motivo:** o wizard é o primeiro lugar do produto onde a UI teria um motivo
"razoável" de persistir ou chamar rede direto - é o fluxo nativo de
qualquer onboarding. Decidir a regra agora, antes da Fase 4/5 implementar
qualquer passo, evita que a exceção pareça aceitável e vire precedente para
outras telas (a própria tela de Configurações, mais adiante) furarem a
regra de UI sem request. Tratar como "wizard multi-step" desde já (em vez
de só "tela de Wi-Fi") evita reabrir esta decisão a cada novo passo
adicionado. A tela de Boot não tinha esse risco - só ficou registrada aqui
pra fechar a dúvida junto.

## ADR-0018 - LVGL real via bsp_display_start() e IBoard::lock()/unlock()
**Decisão:** `WaveshareBoard` usa `bsp_display_start()` (a função tudo-em-um
do BSP oficial) em vez de `bsp_display_new()`/`bsp_touch_new()` separados
(usados no harness da Fase 0). `bsp_display_start()` já inicializa o LVGL,
registra o touch GT911 como input device automaticamente e sobe a própria
task de LVGL do BSP - o firmware não precisa de uma `lvgl_task` própria.
Como LVGL não é thread-safe, qualquer código fora dessa task que toque
`lv_obj_*` precisa do mutex do BSP (`bsp_display_lock`/`unlock`).
`IBoard` ganhou `lock(uint32_t timeout_ms)`/`unlock()` (primitivos, sem tipo
do LVGL/BSP no header) para expor esse mutex sem vazar dependência real -
`MockBoard` implementa como no-op, `WaveshareBoard` chama o BSP.
`app_main.cpp` envolve a chamada de `ui_dispatcher.process_pending()`
(que aciona `HomeScreen::render`) com `board.lock()`/`unlock()`.
`HomeScreen` ficou escopada ao que `AppState` tem hoje (relógio + mercado +
status) - sem clima/agenda/player/cenas do protótipo de referência, que
dependem de providers/services que ainda não existem.

**Achado real (causa raiz de um boot-loop):** ao linkar o BSP gráfico
completo (LVGL + `esp_lvgl_port` + `esp_lcd_ek79007` + `esp_lcd_touch_gt911`
+ `esp_codec_dev`), o `.bss` do firmware cresceu ~123KB sobre o harness da
Fase 0. O maior contribuinte (64KB) é o alocador padrão do LVGL
(`LV_USE_BUILTIN_MALLOC`), que reserva um array estático (`work_mem_int`,
`lv_mem_core_builtin.c`) em RAM interna **desde o boot**, independente de
LVGL estar em uso. Isso reduziu a região de RAM interna/DMA disponível bem
no início do boot, exatamente quando o construtor do ESP-Hosted
(`__attribute__((constructor))`, roda **antes do `app_main`**, antes até da
criação da própria main task) tenta alocar seu mempool SDIO + 4 threads -
causando primeiro `assert failed: sdio_mempool_create` e, mesmo reduzindo as
filas do SDIO (`CONFIG_ESP_HOSTED_SDIO_TX/RX_Q_SIZE=10`, mitigação
documentada no próprio `esp_hosted`), ainda sobrava pouca margem e a criação
da main task em si falhava (`esp_startup_start_app` assert). Trocar para
`CONFIG_LV_USE_CLIB_MALLOC=y` (LVGL usa malloc/free padrão, alocação
dinâmica sob demanda, não uma reserva fixa de 64KB) resolveu - confirmado
via `riscv32-esp-elf-size` (`.bss` caiu exatamente ~64KB) e validado na
placa física sem reset.
**Motivo:** o tamanho do buffer de display (full-frame/double-buffer em
PSRAM, tentado primeiro) e a ordem de bring-up (display vs. rede) foram
descartados como causa - ambos teoricamente plausíveis, ambos testados e
sem efeito, porque o problema real acontecia **antes** de qualquer código
do app rodar. Medir `.bss` direto (`riscv32-esp-elf-size` + `nm
--size-sort`) achou a causa real em minutos onde reordenar código não
ajudava; documentar isso evita repetir a mesma investigação se outro
componente futuro reintroduzir uma reserva estática grande.

## ADR-0019 - Implementação do wizard de onboarding (payload rico sobre EventBus int32)
**Decisão:** `EventBus::Event` só carrega um `int32_t` de payload (decisão
original, ADR implícita nos primeiros componentes) - insuficiente para a
"intenção" do wizard (ADR-0017), que precisa transportar nome, SSID, senha,
timezone etc. Em vez de estender `Event` com um payload genérico/variant,
a UI escreve a submissão no próprio `StateStore`:
`StateStore::submit_onboarding(const OnboardingSubmission&)` grava o struct
em `AppState::onboarding.pending_submission` e publica
`EventType::OnboardingStepSubmitted` (i32 = step) - só `SetupService`
assina esse evento e lê o `pending_submission`. O caminho de volta usa o
mesmo padrão dos outros services: `set_onboarding_step`/`set_wifi_status`/
`set_preferences`/`set_onboarding_needed`, cada um publicando
`EventType::OnboardingStateChanged` para a UI re-renderizar - simetria com
`set_clock`/`set_market`/`ClockUpdated`/`MarketUpdated` já existentes.
`SetupService` é o único a abrir NVS e chamar
`esp_wifi_set_config`/`esp_wifi_connect`; os handlers de `WIFI_EVENT`/
`IP_EVENT` são funções livres (não métodos estáticos) no `.cpp`, para que
`setup_service.hpp` continue sem tipos do ESP-IDF (mesmo padrão de
`waveshare_board.hpp`/ADR-0018). O passo `Wifi` só avança para `Timezone`
quando `IP_EVENT_STA_GOT_IP` chega de fato - diferente dos outros passos
(nome, timezone, formato de hora, tema), que avançam otimisticamente ao
serem persistidos, porque só o Wi-Fi depende de um resultado de rede real.
`WizardScreen` recebe a função de submissão por injeção no construtor
(`SubmitFn`, mesmo padrão de `UiDispatcher::RenderFn`) em vez de depender de
`StateStore` diretamente - mantém a tela só lendo `AppState` no `render()`.
**Motivo:** ADR-0017 fixou a regra (UI nunca persiste/chama hardware) mas
deixou o "modelo de dados" como detalhe de implementação da Fase 4/5.
Reaproveitar `StateStore` em vez de estender `Event` evita um tipo de
payload genérico (`std::any`/union) cedo demais - todo outro fluxo de dados
do app já passa por `StateStore`, então a UI "publicar a intenção" como uma
escrita no `StateStore` (que por sua vez publica o evento) é consistente
com o resto da arquitetura, não uma exceção. O avanço assíncrono do passo de
Wi-Fi (vs. otimista nos demais) evita que o wizard avance para Timezone
achando que conectou quando na verdade falhou.

## ADR-0020 - Wizard: lista de redes escaneadas (Wi-Fi) e timezone por dropdown
**Decisão:** o passo de Wi-Fi do wizard deixa de pedir SSID digitado: ao
entrar no passo, a `WizardScreen` dispara automaticamente um pedido de scan
(`StateStore::request_wifi_scan()`, mesmo padrão de "intenção" de
`submit_onboarding` - UI nunca chama `esp_wifi_*` direto) e mostra a lista
de redes encontradas (`lv_list`, de-duplicadas por SSID com o RSSI mais
forte, ordenadas por sinal, em `SetupService::handle_wifi_scan_request()`).
Tocar numa rede mostra só o campo de senha (SSID já preenchido) - sem
digitar o nome da rede. Há "Atualizar lista" (rescan manual) e "Voltar à
lista" (troca de rede sem reescanear, usa um cache local na
`WizardScreen`). O passo de Timezone troca o texto livre por um `lv_dropdown`
com uma lista curada (`America/Sao_Paulo`, `America/Manaus`,
`America/Rio_Branco`, `America/Noronha`, `UTC`) pré-selecionando o que já
estiver salvo em `UserPreferences::timezone`, se bater com a lista.
O `esp_wifi_scan_start(..., block=true)` e o `esp_wifi_connect()` continuam
bloqueantes (chamados de `SetupService`) - por isso a "intenção" da UI
(scan ou submit) é sempre desviada pela `action_queue` genérica de
`app_main.cpp` (fila de `std::function<void()>*`, generalizada do
`onboarding_queue` específico da ADR anterior) antes de chegar em
`StateStore`, nunca chamada direto do callback de toque do LVGL.
**Motivo:** digitar SSID/senha à mão na placa é o ponto de maior atrito e
erro do onboarding (typo no SSID trava o wizard sem dar pista do motivo) -
escanear e listar elimina essa classe de erro. Timezone livre tem o mesmo
problema (erro de digitação produz uma string que `ClockService` não
reconhece silenciosamente); uma lista curada fixa isso sem precisar de uma
base de dados de timezones completa (fora de escopo do MVP, já registrado
em `app_state.hpp`). Generalizar a fila de ação (em vez de manter duas
filas, uma por tipo de intenção) evita duplicar o mecanismo de defer-off-
da-lvgl_task para cada nova ação que `SetupService` passe a precisar.

## ADR-0021 - NTP real (pré-requisito de HTTPS) e CoinGeckoProvider
**Decisão:** `CoinGeckoProvider` (novo, `providers/coingecko_provider.cpp`)
substitui o `MockMarketProvider` em `app_main.cpp`: uma chamada HTTPS em
lote a `GET /api/v3/simple/price?ids=bitcoin&vs_currencies=usd,brl&
include_24hr_change=true` (ADR-0006 - 60s/6 req-min já impostos pelo
`RequestOrchestrator`, inalterado), via `esp_http_client` +
`esp_crt_bundle_attach` (bundle de CAs já habilitado no sdkconfig) + cJSON.
`usd_brl` é triangulado (`brl/usd` da mesma resposta), não uma fonte de
forex dedicada ainda - ver nota de escopo abaixo.
Validação de certificado TLS depende do relógio do sistema estar correto, e
não há RTC com bateria nesta placa (Fase 0) - sem isso o relógio fica perto
da época Unix e a validação do certificado falharia. Por isso, junto com o
provider: `SetupService::init()` chama `esp_netif_sntp_init()` (config com
`start=false`, `pool.ntp.org`) e `on_wifi_connected()` chama
`esp_netif_sntp_start()` (fire-and-forget, não bloqueia). `ClockService`
deixa de ser um contador mock isolado: em cada tick lê `time(nullptr)` -
se o epoch ainda estiver perto de 1970 (< 2023-11-14, escolhido só como
"claramente antes de qualquer deploy real"), mantém o avanço do relógio
mock seedado (`ADR-0009`); uma vez sincronizado, usa `localtime_r` direto
(`clock.synced=true`). O fuso aplicado vem de `UserPreferences::timezone`
via `setenv("TZ", ...)` + `tzset()`, mapeando as 5 strings curadas do
dropdown (ADR-0020) para regras POSIX TZ (`BRT3`, `AMT4`, `ACT5`, `FNT2`,
`UTC0`) - não há tzdata IANA embarcada, esse mapeamento manual é todo o
"banco de fusos" do MVP.
**Motivo:** HTTPS sem hora certa falha silenciosamente ou de forma confusa
(erro de certificado sem relação aparente com o relógio) - resolver isso
antes do `CoinGeckoProvider` evita depurar um "bug de rede" que na
verdade é falta de NTP. Triangular `usd_brl` em vez de mockar foi escolhido
porque o dado vem de brinde na mesma resposta (nenhum custo extra de
request) e é estritamente melhor que um valor fixo, mesmo sendo uma
aproximação - a fonte de forex dedicada (ROADMAP Fase 5) substitui isso
depois sem mudar a interface `IMarketProvider`. `clock_service.cpp` e
`coingecko_provider.cpp` entraram no `SKIP_FILES` do `host_check.sh`:
o primeiro por `setenv`/`localtime_r` (POSIX, presentes no newlib do
ESP-IDF mas ausentes na libc do MinGW/MSYS2 usada no host), o segundo por
depender de `esp_http_client`/mbedtls/cJSON reais.

## ADR-0022 - Reconexão automática de Wi-Fi com retry limitado a cada boot
**Decisão:** `SetupService::init()` agora lê `wifi_ssid`/`wifi_password` da
NVS e chama `esp_wifi_set_config`/`esp_wifi_connect` automaticamente em
**todo boot** quando `onboarding_done=1` - antes, esse par só era chamado
uma vez, durante o passo Wi-Fi do wizard, então qualquer boot depois do
onboarding ficava com o transporte ESP-Hosted/SDIO de pé mas sem associar a
nenhum AP. Além disso, `on_wifi_disconnected()` ganhou um retry limitado
(até 5 tentativas, `wifi_retry_count_`, resetado em `on_wifi_connected()`):
testado na placa física, a primeira tentativa de `esp_wifi_connect()` logo
após o boot falhou e desconectou (~1.4s depois) mesmo com credenciais
corretas - a 3ª tentativa (2º retry) conectou e obteve IP normalmente.
**Motivo:** sem o auto-reconnect, `CoinGeckoProvider`/`OpenMeteoProvider`
(ADR-0021/0022) nunca teriam rede real em boots subsequentes ao onboarding -
só funcionariam durante a janela do wizard, o que não é o comportamento
esperado de um produto. O retry foi necessário porque, na prática, a
primeira tentativa de associação logo após `WaveshareBoard::bring_up()`
nem sempre é bem-sucedida mesmo com credenciais válidas (motivo exato não
isolado - possivelmente o link ESP-Hosted/SDIO ainda estabilizando) -
retries indefinidos seriam arriscados se a credencial estiver mesmo errada
(loop de log/rádio sem fim), por isso o limite de 5 antes de marcar
`WifiConnectStatus::Failed` definitivamente.

## ADR-0023 - OpenMeteoProvider (clima real, Brasília fixo)
**Decisão:** `OpenMeteoProvider` (novo, `providers/open_meteo_provider.cpp`)
implementa `IWeatherProvider` (nova interface, espelha `IMarketProvider`):
uma chamada HTTPS a `GET /v1/forecast` da Open-Meteo (sem API key, ao
contrário do CoinGecko) pedindo `temperature_2m`, `relative_humidity_2m`,
`weather_code` e `wind_speed_10m` atuais. Localização fixa em Brasília, DF
(`-15.78, -47.93`) - ainda não há passo de localização no wizard.
`WeatherService` (novo, espelha `MarketService`) usa
`DataDomain::Weather` do `RequestOrchestrator`, já configurado desde a
Fase 2 (10min de intervalo, 6 req-min) - a política não mudou, mas
`RequestOrchestrator::note_request()` ganhou um parâmetro `success` (default
`true`): antes, mesmo uma busca que falhasse (ex.: sem rede ainda no boot)
"gastava" o intervalo completo (60s no mercado, **10min no clima**) antes da
próxima tentativa - com clima isso significava esperar 10 minutos pelo
primeiro dado real se a primeira tentativa coincidisse com a janela sem
Wi-Fi associado ainda. Agora `last_request_ms` só avança em sucesso;
`calls_in_window` continua contando em qualquer tentativa, então o budget de
6 req-min (ADR-0006) ainda limita retries rápidos em caso de falha
persistente. O código WMO de clima (`weather_code`) é reduzido a um
`WeatherCondition` (Clear/Cloudy/Fog/Drizzle/Rain/Snow/Thunderstorm/Unknown)
em `OpenMeteoProvider`, guardado em `WeatherSummary` - ainda sem tela
dedicada (`screen_clima.c` é trabalho futuro, `docs/design/README.md`).
**Motivo:** mesma lógica do CoinGecko (ADR-0006/0021) - Open-Meteo é
gratuito, sem chave, e já reusa o NTP/TLS resolvidos para o provider de
mercado. Fixar a localização em vez de esperar uma feature de geolocalização
no wizard mantém o escopo pequeno (decisão registrada com o usuário:
Brasília, DF) - trocar para uma localização configurável é um passo de
wizard futuro, não uma mudança de arquitetura (a interface
`IWeatherProvider` não precisa mudar).

## ADR-0024 - Redesign v2 (Boot/Wizard/Home) fiel ao design + MainShell
**Decisão:** o material de design em `docs/design/lvgl_export_reference` foi
atualizado pelo usuário (v2: `screen_boot.c`, `screen_setup.c` novos;
`novapanel_theme.h` com fontes maiores para 7"; `novapanel.c` com rail+topbar
icon-only). O firmware foi reportado por inteiro:
- **OnboardingStep** passa de 5 para 4 passos -
  `DisplayName -> Wifi -> TimezoneAndFormat -> Confirmation -> Done` - sem
  passo de Tema (`ThemeMode` continua existindo em `UserPreferences`, só não
  é mais setado pelo wizard; fica para uma futura tela de Configurações).
  `TimezoneAndFormat` funde os dois em uma submissão só.
- **`WizardScreen`** vira chrome fixo (progress bar + header + footer
  Voltar/Continuar) com os 4 painéis construídos uma vez e
  mostrados/escondidos (não mais destruídos/reconstruídos por passo). O
  passo de Wi-Fi mostra lista + painel de senha **lado a lado**
  (decisão do usuário), não mais sequencial.
- **`BootScreen`** ganha badge "NP", barra de progresso real (baseada em
  `lv_tick_elaps()` contra `duration_ms`, não no fake random da referência -
  o bring-up de hardware já terminou de verdade antes da splash aparecer,
  então não há progresso real "por etapa" pra mostrar) e botão "Pular"
  funcional (`BootScreen::SkipFn`, mesmo padrão de injeção do
  `WizardScreen::SubmitFn`).
- **`MainShell`** (novo) porta `novapanel.c`'s rail+topbar+dots - chrome
  persistente que `HomeScreen` (e futuras telas) montam dentro de
  `MainShell::content()` em vez de cada tela ser sua própria
  `lv_screen_load()`. Decisão do usuário: a trilha (7 ícones) e os botões
  da topbar ficam visualmente completos, mas só "Início" é navegável - os
  outros são inertes até a tela correspondente existir (Agenda, Mercado
  dedicado, Casa, Alarmes, Clima dedicado, Timer, Notificações,
  Configurações). Desvio da referência: a trilha começa **fechada** e
  funciona como *drawer* (abre tocando o ícone ☰ na topbar ou arrastando da
  esquerda pra direita; fecha tocando de novo ou arrastando pra esquerda,
  via `LV_EVENT_GESTURE`/`lv_indev_get_gesture_dir()`) em vez de
  permanentemente visível empurrando o conteúdo.
- Fontes `NP_F_XS`/`NP_F_SM` da referência subiram (10→12, 12→14) - todo
  `home_screen.cpp` já portado foi reajustado pra bater.
**Motivo:** o usuário pediu fidelidade literal ao material de design, não
uma aproximação - duas rodadas de correção (Home com card de clima solto em
vez de aninhado no card do relógio; depois Boot/Setup totalmente
diferentes da nova v2) confirmaram que "parece com o design" não é
suficiente quando o design em si é a fonte de verdade. A trilha
fechada-por-padrão foi pedido explícito do usuário (a versão sempre-visível
do design ocupa espaço de tela permanentemente sem nenhuma tela real atrás
da maioria dos ícones ainda).

## ADR-0025 - Wi-Fi list sem rebuild no toque (stack overflow) + busca manual
**Decisão:** duas correções relacionadas, achadas testando o wizard na
placa física:
1. `***ERROR*** A stack overflow in task taskLVGL has been detected` - o
   toque numa rede da lista chamava `refresh_wifi_network_list()`, que
   fazia `lv_obj_clean()` + recriava todas as linhas (até 24 redes × ~5
   widgets) **dentro do próprio callback de toque**, rodando na task
   `taskLVGL` (a task do BSP que processa toque + despacha eventos). Agora
   `WizardScreen` pré-constrói até `kMaxWifiRows` (24) linhas **uma vez**
   (`build_wifi_panel()`); `refresh_wifi_network_list()` (chamada só de
   `render()`, fora do toque, quando o resultado do scan muda) atualiza
   texto/visibilidade dessas linhas já existentes; tocar numa rede só roda
   `update_wifi_selection_ui()` (recolorir + mostrar/escurecer checkmark
   nas linhas já existentes, sem criar nada). Defesa adicional:
   `WaveshareBoard::bring_up_display_and_touch()` passou a usar
   `bsp_display_start_with_config()` (em vez de `bsp_display_start()`) só
   pra subir `lvgl_port_cfg.task_stack` de 7168 (default do BSP) para
   16384 bytes - mantendo buffer/double_buffer/flags idênticos ao default
   (não reabre a investigação de PSRAM da ADR-0018).
2. O scan de Wi-Fi deixou de disparar automaticamente ao entrar no passo -
   só roda quando o usuário toca em "Atualizar lista"
   (`on_wifi_rescan`/`StateStore::request_wifi_scan()`); o resultado fica em
   cache (`wifi_networks_cache_`) pelo resto da sessão do wizard, mesmo
   navegando pra frente/trás entre passos - mesma política deve valer
   depois pra uma tela de Configurações que edite Wi-Fi.
**Motivo:** (1) é a causa raiz real do "tela azul e reinicia sozinho às
vezes quando toca na tela" reportado pelo usuário - confirmado pelo
backtrace de pânico capturado via serial, não um chute. Pré-construir as
linhas é estritamente melhor que só aumentar a stack (que sozinha mascararia
o desperdício sem resolver a causa); a stack maior fica como margem de
segurança pra qualquer tela futura igualmente pesada, já que o default de
7KB do BSP nunca foi validado contra uma árvore de UI deste tamanho.
(2) foi pedido explícito do usuário ("isso só precisa acontecer uma vez...
e somente se a pessoa solicitar") - evita também gastar o budget de
sensoriamento de rede sem necessidade.

## ADR-0026 - ForexProvider dedicado (AwesomeAPI) para USD/BRL

**Decisão:** nova interface `IForexProvider` (espelha `IMarketProvider`/
`IWeatherProvider`) + `ForexProvider` (`providers/forex_provider.cpp`): uma
chamada HTTPS a `GET /json/last/USD-BRL` da AwesomeAPI
(`economia.awesomeapi.com.br`, sem API key, mesma convenção do CoinGecko/
Open-Meteo), parseando o campo `bid` (vem como string JSON, não número).
`ForexService` (novo, espelha `MarketService`/`WeatherService`) usa
`DataDomain::Forex` do `RequestOrchestrator` - já existia uma policy pronta e
sem uso (120s, 6 req-min) desde a Fase 2, só ativada agora. Isso substitui a
triangulação anterior (`usd_brl = brl/btc_usd` a partir do mesmo snapshot do
CoinGecko, ver ADR-0006), que era um placeholder explícito no código
aguardando essa fonte dedicada. `CoinGeckoProvider` voltou a pedir só
`vs_currencies=usd` (não busca mais `brl`).

Como agora duas fontes independentes escrevem no mesmo `MarketSummary`,
`usd_brl`/`usd_brl_valid`/`usd_brl_stale`/`usd_brl_source`/
`usd_brl_last_update_ms` ficaram separados de `btc_usd`/`btc_change_24h`/
`valid`/`stale`/`source`/`last_update_ms`, e `StateStore::set_market()`
passou de um replace completo do struct para um merge parcial (preserva os
campos `usd_brl_*` ao atualizar os campos `btc_*`, e vice-versa via
`set_usd_brl_rate()`) - um fetch não pode apagar o resultado do outro.
`HomeScreen` mostra "Dólar" condicionado a `usd_brl_valid` e "Bitcoin"
condicionado a `valid`, independentemente.

**Motivo:** item já registrado como pendência no ROADMAP (Fase 5: "fonte
USD/BRL dedicada") - a triangulação via CoinGecko funcionava mas acoplava a
cotação do Dólar à disponibilidade/rate-limit do provider de Bitcoin sem
necessidade. AwesomeAPI é brasileira, gratuita, sem chave e item dedicado
(consistente com a escolha do Open-Meteo para clima, ADR-0023). O merge
parcial no `StateStore` é a correção mínima necessária para dois services
independentes coexistirem no mesmo struct sem se sobrescreverem - dividir
`MarketSummary` em dois structs separados (mercado vs. câmbio) resolveria o
mesmo problema com mais clareza, mas foi descartado por ora para não
propagar a mudança em todos os pontos que já leem `state.market` (escopo
mínimo, ver `home_screen.cpp`/`market_service.cpp`).

## ADR-0027 - Cache local em LittleFS (Fase 6)

**Decisão:** novo componente `firmware/components/cache/` com `CacheStore`
(`mount()`/`write()`/`read()` genéricos, sem dependência de `models/` - não
conhece `WeatherSummary`/`MarketSummary`). Monta LittleFS (dependência
gerenciada `joltwallet/littlefs`) na partição `storage` já existente
(`partitions.csv`, 1MB, tipada `data`/`spiffs` mas nunca usada por nada no
código - o BSP Waveshare tem um `bsp_spiffs_mount()` opcional, nunca
chamado) - não precisou mudar `partitions.csv`: LittleFS acha a partição
pelo *label* (`ESP_PARTITION_SUBTYPE_ANY`), não pelo subtype declarado.
Escrita atômica via `<key>.tmp` + `rename()` (ADR-0015); cada blob tem um
header (`magic` + `schema_version` + `payload_size`) - leitura que não bate
qualquer um dos três é tratada como "sem cache" (sem distinguir corrupção
de versão diferente, sem migração - ver nota de escopo). `write()`/`read()`
retornam `bool` em vez de `Result<T>`/`Status` (convenção do projeto): a
API é deliberadamente genérica (`void*`+`size_t`), `Result<T>` não encaixa
bem nesse formato, e `false` em `read()` é o caminho normal (cache vazio na
primeira vez), não uma condição excepcional.

Cache dividido por domínio (`weather`, `market_btc`, `market_forex`) em vez
de um blob único, porque `MarketService` e `ForexService` já escrevem
campos independentes do mesmo `MarketSummary` em memória (ADR-0026) - um
arquivo compartilhado reproduziria em disco o mesmo bug de sobrescrita que
foi corrigido no `StateStore::set_market()`. `WeatherService`/
`MarketService`/`ForexService` ganharam uma referência a `CacheStore`: no
`init()` tentam ler do cache e, se existir, semeiam o `StateStore` com
`valid=true, stale=true, source=DataSource::Cache` (a Home mostra o último
dado bom em vez de "--"/"sem dados ainda" antes mesmo do Wi-Fi conectar);
depois de cada fetch real bem-sucedido, gravam no cache. `app_main.cpp`
monta o `CacheStore` logo após `board.bring_up()` e usa o resultado pra
preencher `SystemStatus.cache_ready` (antes hardcoded `false`).
`home_screen.cpp` passou a mostrar o sufixo `" (cache)"`/`" (mock)"` pro
Bitcoin e pro clima, mesmo padrão que já existia pro Dólar.

**Nota de escopo:** ADR-0015 menciona "migração no boot", mas esta é a
primeira versão de schema - não há nada anterior pra migrar de. Por ora,
incompatibilidade de schema é só descartada (trata como cache ausente);
lógica de migração real fica para quando houver de fato uma v2 de algum
schema.

**Motivo:** item já registrado como pendência no ROADMAP (Fase 6) e
decidido na ADR-0015 (LittleFS, não SPIFFS, por wear leveling + escrita
atômica). Reusar a partição `storage` existente evita reparticionar a
flash; dividir por domínio é a mesma lição já aprendida e documentada na
ADR-0026, aplicada também à persistência em disco.
