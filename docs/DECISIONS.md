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
