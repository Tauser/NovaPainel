# NovaPanel — Análise Arquitetural Independente

> Revisão técnica profunda do repositório em 2026-07-02, na perspectiva de um
> Principal/Staff Engineer externo. Baseada em leitura direta do código ativo
> (`firmware/`), da documentação canônica (`docs/`), dos ADRs e do histórico git.
> Fatos, inferências, hipóteses e opiniões estão marcados como tal.

---

## 1. Veredito executivo

**A base é promissora e a disciplina de engenharia é real — mas o repositório
está mentindo para si mesmo sobre o próprio estado, e a base atual ainda é um
sistema pessoal bem-feito, não um produto premium.**

Leitura sincera em cinco frases:

1. O núcleo arquitetural (EventBus, StateStore, UiDispatcher,
   RequestOrchestrator, NetworkWorker) é pequeno, legível, coerente com os ADRs
   e implementa de verdade o fluxo `Provider → Service → StateStore → EventBus
   → UI`. Isso é raro em firmware hobbyista e é o maior ativo do projeto.
2. O conhecimento de hardware acumulado (contenção MSPI/PSRAM vs MIPI-DSI,
   picos de TLS, erase de flash travando o framebuffer) está codificado em
   decisões e comentários — é patrimônio caro de reproduzir e está bem
   preservado.
3. Porém, há **drift documental grave**: `CLAUDE.md` descreve um firmware
   ("Fase 11 concluída, MVP em produção", `board/` com `IBoard`/`WaveshareBoard`,
   `Result<T>`, telas como classes) **que não existe no tree ativo**. O firmware
   real é um reboot parcial (o próprio `ROADMAP.md` diz Fases 2, 3, 5, 6 e 7
   pendentes). Qualquer engenheiro ou agente que confie no `CLAUDE.md` vai
   trabalhar contra uma realidade falsa.
4. A camada de abstração de hardware prometida (ADR-0003) foi abandonada no
   reboot: `app_main.cpp` (851 linhas) fala direto com o BSP e contém ~300
   linhas de driver de áudio inline. Os "ports and adapters" dos providers
   também não existem — services dependem de providers concretos.
5. Para um produto premium faltam fundamentos não-negociáveis: OTA (a tabela de
   partições é factory-only), profundidade de teste (1 arquivo de teste, 121
   linhas, zero teste de parsing de providers), telemetria de operação contínua
   e desacoplamento do produto de um único usuário brasileiro (pt-BR, Brasília,
   BTC/USD-BRL hardcoded).

Compatível com produto premium? **Ainda não.** Compatível com virar um? **Sim,
sem reescrita** — os problemas são de completude, governança e endurecimento,
não de fundação errada.

---

## 2. Entendimento do sistema

### O que o sistema é (fato)

Smart display 7" local/offline-first sobre Waveshare ESP32-P4-WIFI6-Touch-LCD-7B
(P4 = aplicação + display MIPI-DSI 1024×600; C6 = rádio Wi-Fi via
ESP-Hosted/SDIO). Firmware ESP-IDF + LVGL em C++17, monorepo com `docs/`
canônico, `shared/` (contratos JSON para um server futuro), `skills/` (workflow
para agentes) e um tree legado de 1,1 GB mantido como referência.

Funcionalmente hoje: relógio (RTC→NTP), clima (Open-Meteo), BTC (CoinGecko,
incl. OHLC/candles), USD/BRL (AwesomeAPI), onboarding Wi-Fi com wizard,
notificações com toast, settings (brilho, tema, formato de hora, volume com
beep ES8311), cache offline em LittleFS, telas restantes em placeholder.

### Blocos principais e relações (fato)

```text
main/app_main.cpp        wiring de TUDO + loop de 200ms + driver de áudio inline
core/                    EventBus (sync), StateStore (mutex+snapshot),
                         UiDispatcher (fila+coalescing), RequestOrchestrator
                         (política/rate-limit/circuit breaker), ServiceManager
models/                  AppState puro, sem IO — host-checkável
services/                Clock, Setup (Wi-Fi/NVS/NTP), System, Notification,
                         Market, Forex, Weather + NetworkWorker (task única de rede)
providers/               CoinGecko, OpenMeteo, Forex — concretos, parse cJSON
cache/                   CacheStore: blobs binários versionados, escrita atômica
utils/                   http_client (mutex global, buffer SRAM 48KB)
ui/                      API C-style np_* + telas com lv_obj_t estáticos por arquivo
```

Fluxo real de dados, verificado no código: `NetworkWorker` (task própria,
serializada) escolhe o domínio "due" de maior prioridade via orquestrador →
chama `Service::refresh()` → provider faz HTTP+parse → service grava no
`StateStore` → publish síncrono no `EventBus` → `UiDispatcher` enfileira →
loop principal drena sob `bsp_display_lock` e repinta via `np_update_*`.
Intenções da UI voltam por callbacks `np_bind_*` → `enqueue_action` → fila →
loop principal → `StateStore`. **O desenho documentado é o desenho
implementado** — isso merece registro.

### Fronteiras ambíguas ou mal definidas (fato/inferência)

- **`main/` não é wiring, é um componente**: além de montar o grafo, contém o
  driver ES8311/I2S completo, a política de flags de repintura, e o loop de
  render. É a fronteira mais violada do sistema.
- **`board/` não existe** apesar de constar em `ARCHITECTURE.md` §4 e ADR-0003.
  O BSP vaza para `main/` e o display lock vaza semanticamente para o áudio.
- **Dupla numeração de fases**: CLAUDE.md/ADRs falam "Fase 11 concluída";
  ROADMAP renumerado diz "Fase 4 feita, 2/3/5/6/7 pendentes". Dois relógios de
  projeto correndo ao mesmo tempo.
- **`shared/`** define contratos para um server que não existe — pequeno, mas é
  superfície especulativa a manter sincronizada com structs por disciplina manual.

---

## 3. Avaliação arquitetural profunda

### Onde a arquitetura é boa como arquitetura (fato + opinião)

- **Autoridade de estado única e real.** Toda mutação passa pelo `StateStore`;
  a UI publica intenção e lê snapshot. Não encontrei nenhum request ou
  persistência disparada pela UI. O invariante nº 1 do projeto se sustenta sob
  inspeção — não é só slogan de doc.
- **RequestOrchestrator é a peça mais madura.** Política por domínio
  (intervalo, prioridade, rate limit), circuit breaker com backoff exponencial
  + jitter (xorshift próprio, sem dependência), transição
  Closed→Open→HalfOpen correta, e é a única peça com teste unitário de
  comportamento. É gatekeeper puro (não executa) — separação decisão/execução
  correta.
- **ADR-0035 (NetworkWorker) é uma decisão exemplar.** Diagnóstico medido
  (3 handshakes TLS simultâneos ≈ 390 KB, RAM interna a 173 KB, circuit breaker
  abrindo em cascata), correção em duas fases com validação de campo entre
  elas, e trade-off explícito (keep-alive descartado com justificativa
  quantitativa). É assim que se decide arquitetura.
- **Modelos limpos.** `AppState` é dado puro com `valid/stale/source/
  last_update_ms` em todo domínio — o "stale explícito" existe de fato e a UI
  o representa. `OhlcSeries` mantida fora do snapshot para não inflar cópias —
  consciência de custo de memória no design do modelo.

### Onde é frágil (fato)

- **`app_main.cpp` é um god-file em formação.** 851 linhas: init de display,
  driver de áudio (~300 linhas), transporte Wi-Fi, hooks do cJSON, wiring de 12
  objetos, 15 binds de callback, política de coalescing por flags e o loop de
  render. Cada feature nova adiciona binds + flags + ramos no callback de
  render aqui. Hoje é legível; em 2× o número de telas será o gargalo de
  manutenção do projeto.
- **Ports-and-adapters é aspiracional, não implementado.** `MarketService`
  depende de `CoinGeckoProvider` concreto; `WeatherService` de
  `OpenMeteoProvider` concreto. Existe um `i_forex_provider.hpp` órfão — indício
  de intenção não concluída. Consequência prática: impossível testar services
  no host com provider fake, impossível trocar de API sem tocar o service, e a
  doc (`PLANEJAMENTO.md` §2: "ports and adapters") descreve algo que o código
  não faz.
- **Coalescing duplicado em duas camadas.** O `UiDispatcher` coalesce eventos
  por tipo; em seguida `app_main` re-coalesce com cinco booleanos
  (`home_needs_update`, etc.) dentro do callback de render. Duas soluções para
  o mesmo problema, em camadas diferentes, com regras ligeiramente diferentes
  (ex.: `NotificationCreated` marca home+shell+settings+weather). Funciona, mas
  é a assinatura clássica de responsabilidade sem dono.
- **Complexidade especulativa pontual.** `DataDomain` tem 9 domínios com
  políticas configuradas (Calendar, Photos, Notifications, System,
  MarketRealtime) dos quais 4 têm fetcher real. `shared/schemas` versiona um
  protocolo sem implementação em nenhuma ponta. Custo baixo, mas é inventário
  morto que a doc trata como vivo.

### Onde a complexidade é justificada (opinião arquitetural)

As três "gambiarras" mais estranhas do código são, na verdade, engenharia
correta para a física desta placa, e estão documentadas no ponto de uso:

- mutex global de HTTP (1 TLS por vez) — evita colapso de RAM interna;
- corpo HTTP e nós cJSON forçados em SRAM interna — evita contenção de
  barramento com o DMA do display (DSI underrun / flash branco);
- throttle de escrita do cache (30 min) — erase de flash no MSPI compartilhado
  trava a leitura do framebuffer em PSRAM.

Nenhuma dessas é sofisticação gratuita. O problema não é a existência delas —
é que esse invariante físico ("SRAM interna e barramento MSPI são o recurso
escasso do sistema") está espalhado em comentários de 4 arquivos em vez de ser
um contrato central de alocação. Ver §10.

---

## 4. Escalabilidade e desacoplamento

### O que escala bem (fato)

Adicionar um domínio de dados novo tem receita clara e repetível (a skill
`novapanel-add-service` existe para isso): enum + política + model + provider +
service + blob de cache + setter no StateStore + EventType + wiring. São ~9
pontos de toque, todos rasos e mecânicos. O `NetworkWorker` absorve o novo
domínio sem task nova — o modelo de concorrência **não cresce** com o número de
serviços. Isso é desacoplamento real, não só pastas separadas.

### Acoplamentos escondidos (fato — os importantes)

1. **Display lock = mutex de I2C.** GT911 (touch) e ES8311 (codec) compartilham
   o barramento I2C, e o polling de touch roda dentro do `lv_timer_handler`
   segurando o display lock; logo, qualquer registro do codec precisa adquirir
   o *display lock*. Está comentado, mas é um invariante inter-domínio
   (áudio ↔ render ↔ touch) que nenhum tipo do sistema expressa. Quem adicionar
   o terceiro periférico I2C sem ler aquele comentário quebra o sistema.
2. **Fan-out de repintura hardcoded no wiring.** O mapa "evento X invalida
   telas Y,Z,W" vive em `app_main` como cadeia de if/else. Cada tela nova exige
   editar esse mapa; cada evento novo idem. É acoplamento N×M disfarçado de
   configuração.
3. **A UI é um singleton estático.** Telas são arquivos com dezenas de
   `static lv_obj_t*` de escopo de arquivo e uma função `np_update_*` que
   repinta a tela inteira a partir do `AppState` completo. Não há noção de
   tile/card/componente com ciclo de vida próprio. Para o roadmap declarado
   (devices, rotinas, álbum, foco, agenda funcional), esse padrão implica que
   cada tela nova repete todo o boilerplate e que "repintar tudo o que talvez
   tenha mudado" vira o modo default de crescimento — o oposto da UI
   state-driven granular que a doc pede.
4. **Data race documentada como segura.** `MarketService::request_ohlc_period`
   escreve `pending_period_`/`last_ohlc_ms_` a partir do loop principal
   enquanto o `NetworkWorker` os lê em outra task, sem atomics — o header ainda
   afirma "seguro chamar de qualquer task". Na prática o risco é baixo (enum de
   1 byte), mas é UB formal e, pior, é um *precedente*: o projeto não tem regra
   escrita de propriedade de thread por campo, só bons instintos caso a caso
   (SetupService usa atomics; MarketService não).

### Veredicto de escala (opinião)

Para o eixo "mais domínios de dados", a base escala com segurança. Para o eixo
"mais telas e interação rica" — que é exatamente o eixo do produto premium — a
base atual (np_* globais + flags em app_main + repintura total) acumula custo
quadrático de manutenção. É o principal limite estrutural a atacar antes da
Fase 5 do roadmap.

---

## 5. Firmware, UI e fluxo de estado

- **Separação domínio/apresentação: boa, com um vazamento de direção.**
  Services não conhecem LVGL (verificado); a UI não conhece providers. Mas a
  *política de apresentação* (o que repintar quando) vazou para `app_main`, e a
  *formatação* (strings pt-BR, nomes de mês, currying de percentuais) vive
  dentro das telas. Falta uma camada fina de view-model; hoje o `AppState` cru
  é o view-model, o que força cada tela a reinterpretar o domínio.
- **Fluxo de dados: claro e auditável.** Dá para seguir um preço de BTC do
  socket até o label com grep. O EventBus síncrono com payload `int32` é uma
  escolha deliberadamente humilde (ADR-0019 discute isso) e correta para o
  tamanho atual: dados grandes viajam pelo StateStore, eventos são só
  sinalização. Aprovo a escolha.
- **StateStore por cópia de snapshot**: simples e correto, mas cada repintura
  copia o `AppState` inteiro (incluindo vetor de notificações com strings).
  A 5 Hz de tick isso é heap churn constante em um sistema onde SRAM interna é
  o recurso crítico. Hoje inócuo (fato: sem evidência de problema); com estado
  10× maior vira pressão de fragmentação (hipótese). Os acessores granulares
  já existentes (`clock()`, `weather()`...) apontam a direção certa.
- **Loop de 200 ms**: o estado da aplicação atualiza a no máximo 5 Hz, mas
  animações/scroll rodam na `lvgl_task` do BSP — a fluidez de interação não
  depende do loop. Desenho adequado. O `action_queue` de profundidade 4 com
  **drop silencioso** de ações de UI é um bug de UX latente barato de corrigir
  (logar + aumentar, ou bloquear com timeout curto).
- **Sustentabilidade do state model**: a decisão de manter OHLC fora do
  snapshot mostra o critério certo; o mesmo critério precisará ser aplicado a
  agenda/fotos (dados grandes por domínio, referência ou versão no snapshot).

---

## 6. Robustez, confiabilidade e operação

### Acima da média (fato)

- Falha de fetch → recarrega cache com `stale=true` → UI mostra dado com fonte
  explícita. Circuit breaker impede marteladas; backoff com jitter; half-open
  para sondagem. Rede ausente → NetworkWorker dorme; UI segue operável.
- Cache: header com magic/versão/tamanho, escrita tmp+rename (atômica no
  LittleFS), mismatch de schema → ignora sem brickar. NVS com `schema_v`,
  migração de dados legados e caminho para versão futura desconhecida.
- Clock offline-first genuíno: RTC com bateria → hora plausível imediata sem
  rede; NTP refina depois; sem hora plausível, mostra não-sincronizado em vez
  de inventar data.
- Wi-Fi: reconexão com retry limitado por boot (ADR-0022), scan com timeout e
  retry, eventos de driver entregues por atomics e drenados no tick — o padrão
  ISR-context→flag→tick está correto.
- As mitigação de glitches visuais (render parcial em vez de full, throttle de
  flash-erase, JSON em SRAM) foram derivadas de sintomas observados na placa,
  não de cargo cult. Há aqui um nível de acabamento visual raro.

### Abaixo de produto premium (fato)

- **`abort()` se o display falhar no boot** → boot loop de hardware sem retry,
  sem modo degradado, sem contador que pare de tentar. Para um display product
  é discutível, mas premium exige pelo menos backoff de reboot + breadcrumb.
- **Sem OTA.** Partições factory-only. Todo bug em campo = cabo USB. O
  `RELEASE-ROLLBACK.md` é honesto sobre isso, mas o `sdkconfig.prod` habilita
  anti-rollback — configuração que só faz sentido com OTA — sinal de config
  copiada antes do fundamento existir.
- **Tensão coredump**: `FIELD-OPERATIONS.md` baseia a triagem de campo em
  coredump; `sdkconfig.prod` **desliga** coredump-to-flash. Em produção, o
  mecanismo de diagnóstico primário não existe. Contradição não registrada em
  ADR.
- **Observabilidade = logs.** Reset reason e reboot count vão para o estado
  (bom), heap snapshots só em fases de boot. Não há watermark contínuo de
  heap/stack, contadores persistidos de falha por domínio, nem uso do evento
  `ResourceWarning` (definido e nunca publicado — fato). Operação contínua de
  meses é hipótese, não medição: o soak de 72h documentado pertence ao tree
  legado/Fase 0, e o ROADMAP marca a fase de resiliência operacional como
  pendente.

---

## 7. Segurança e qualidade

- **Postura de segurança planejada acima da classe do projeto** (fato):
  Secure Boot v2 + Flash Encryption RELEASE + NVS Encryption + partição
  `nvs_keys` + logs em WARN no prod. TLS com bundle de CAs em todas as APIs.
  Nada de segredo em log (verificado por amostragem). Superfície de entrada é
  mínima: o dispositivo só faz GET de APIs públicas; não há servidor local
  escutando.
- **Mas o build PROD é uma promessa não provada** (lacuna de evidência): não há
  registro de que `sdkconfig.prod` tenha sido flashado e validado *neste* tree
  (o ROADMAP põe hardening PROD na Fase 7, pendente). Flash Encryption RELEASE
  é irreversível por unidade — exatamente o tipo de coisa que precisa de ensaio
  documentado antes de existir "produção".
- **Qualidade de código: alta consistência** (fato): namespace `nova` uniforme,
  RAII/refs em vez de new/delete, `kTag` por arquivo, comentários que explicam
  porquês com números medidos. O padrão "porquê + sintoma + medida" nos
  comentários é o melhor que já se vê em firmware.
- **Higiene de repositório: ruim** (fato): `firmware/build/` com 325 MB dentro
  do working tree, `node_modules` (5,2 MB) dentro de `components/ui`, 1,1 GB de
  tree legado na raiz, `temp_*.txt`/`backup_lines.txt` soltos, `docs/_backup`.
  Nada disso quebra o firmware; tudo isso degrada clone, busca, backup e a
  credibilidade do "monorepo auditável".
- **Teste: o ponto mais fraco da disciplina.** 121 linhas de teste nativo
  cobrindo EventBus/StateStore/Orchestrator/notificações — bons testes, mas
  únicos. **Zero testes de parsing de provider**, que é onde payload externo
  hostil/mudado encontra o sistema (o parse do OHLC, por exemplo, confia em
  índices de array do CoinGecko). O `host_check.sh` depende de `pwsh.exe`
  hardcoded — só roda em Windows/WSL, quebrando a promessa de validação
  portátil e inviabilizando CI Linux barato.

---

## 8. O que está sólido (e por quê de verdade)

1. **O fluxo de autoridade** UI→intenção→StateStore→evento→render. Sólido
   porque é *verificável por inspeção* — as regras não dependem de disciplina
   futura, a topologia dos includes já as impõe (ui/ só REQUIRE models+lvgl —
   fato do CMake).
2. **RequestOrchestrator + NetworkWorker.** Sólido porque nasceu de falha
   medida em campo, tem teste, tem ADR com trade-offs quantificados, e reduz o
   modelo de concorrência de N tasks para 1.
3. **Degradação para cache com stale explícito de ponta a ponta** — modelo,
   cache, service e UI concordam sobre o que é dado velho. É o requisito
   central do produto ("continuar útil sem internet") implementado sem atalho.
4. **Governança por ADR.** 35 ADRs com contexto/decisão/motivo, incluindo
   decisões revertidas e fases de migração. O custo de raciocínio já pago está
   preservado — isso vale mais que o código.
5. **O conhecimento físico da plataforma** (SRAM vs PSRAM vs MSPI vs DSI).
   Sólido porque foi conquistado por depuração real e está escrito onde dói.

## 9. O que está apenas funcionando por enquanto

1. **`app_main.cpp`** — funciona porque o sistema tem 4 domínios e 6 telas
   ativas. Cresce linearmente em binds, flags e ramos por feature; é o arquivo
   que vai transformar toda mudança futura em merge arriscado.
2. **UI de singletons estáticos com repintura total** — imperceptível hoje;
   vira custo de manutenção e de CPU quando agenda/devices/álbum saírem do
   placeholder.
3. **Providers concretos sem interface** — funciona porque cada domínio tem
   exatamente 1 API e a API não mudou. Primeira mudança de API (CoinGecko é
   notório por isso) ou primeiro teste sério de service expõe o acoplamento.
4. **Sincronização por convenção entre tasks** — os casos atuais estão certos
   ou benignos, mas não há regra escrita de ownership; o comentário incorreto
   em `request_ohlc_period` mostra que a convenção já falhou uma vez.
5. **`kBodyCap` 48 KB / truncamento silencioso de resposta HTTP** — o handler
   trunca e loga warning, e o parse do JSON truncado falha "limpo". Funciona,
   mas um payload que cresça (OHLC 365d) degrada para falha permanente do
   domínio com diagnóstico pobre.
6. **Timezone por tabela hardcoded de 5 zonas brasileiras sem regra de DST** —
   correto enquanto o Brasil não tiver DST e o usuário morar nele.

## 10. Riscos estratégicos

1. **Governança documental furada (o maior risco, e é barato).** CLAUDE.md
   afirma um estado de projeto falso; ADRs e ROADMAP usam numerações de fase
   conflitantes; ARCHITECTURE §13 descreve um estado parcial diferente dos dois.
   Num repo operado por agentes de IA — que este repo explicitamente é — doc
   errada não é inconveniente, é *injeção de decisão errada em loop*.
2. **A física da placa como teto de produto.** O barramento compartilhado
   PSRAM/flash/DSI já força: 1 HTTP por vez, JSON em SRAM, cache a cada 30 min,
   sem avoid_tearing. Álbum de fotos, áudio contínuo e OTA — todos no roadmap —
   são *exatamente* as features que mais pressionam esse barramento. Risco: o
   roadmap v1.0 pode ser fisicamente mais caro do que o planejado. Falta um
   documento-orçamento ("budget de banda/RAM por feature") que hoje só existe
   fragmentado em comentários.
3. **Ausência de OTA numa base com Flash Encryption RELEASE.** Combinar
   unidade cripto-lacrada e irreversível com atualização só por cabo é a pior
   combinação possível para campo. OTA precisa preceder qualquer provisionamento
   PROD real.
4. **Fase 5 (telas do MVP) sobre a UI atual.** Construir 5+ telas funcionais no
   padrão atual triplica o custo do refactor de UI que virá inevitavelmente
   depois. É a decisão de sequenciamento mais importante do próximo trimestre.
5. **Bus factor = 1.** Todo o contexto não-escrito (por que o beep segura o
   display lock, por que 400 ms de gap) está na cabeça de uma pessoa e em
   comentários. Os ADRs mitigam; a divergência doc↔código corrói exatamente
   essa mitigação.

## 11. Lacunas de evidência

- **Estabilidade de longa duração do tree atual**: nenhum soak registrado no
  firmware novo (o de 72h foi da Fase 0/legado). Uptime de semanas é hipótese.
- **Build PROD**: nunca comprovadamente flashado/validado neste tree; Secure
  Boot/Flash Enc/NVS Enc são configuração, não capacidade demonstrada.
- **Comportamento com heap degradado**: não há teste ou telemetria de operação
  sob fragmentação/pressão de memória; `ResourceWarning` nunca é emitido.
- **Robustez de parsing**: nenhum teste de payload malformado/truncado/campos
  ausentes para os 3 providers.
- **Touch/latência/fluidez**: não há nenhuma medição registrada (fps, tempo de
  repintura por tela, latência de toque) — as afirmações de fluidez são
  subjetivas.
- **host_check em ambiente não-Windows**: quebrado (dependência `pwsh.exe`);
  logo, "todo componente compila no host" só é verdade na máquina do autor.
- **Rotação PPA + carga de rede simultânea**: as mitigações de underrun são
  recentes (commits do topo); não há registro de validação prolongada pós-fix.

## 12. Próximas decisões recomendadas

**Corrigir imediatamente (dias, custo baixo, valor alto):**

1. Reescrever `CLAUDE.md` para descrever o firmware real (sem board/, sem
   Result<T>, fases do ROADMAP novo) e unificar a numeração de fases em todos
   os docs. É a correção com melhor razão custo/benefício do repositório.
2. Tirar `pwsh.exe` do `host_check.sh` (detectar SO) e adicionar testes de
   parsing dos 3 providers com fixtures de payload real + malformado.
3. `action_queue`: logar drop e aumentar profundidade; corrigir o comentário e
   os campos de `request_ohlc_period` (atomics ou passar pelo caminho da fila).
4. Higiene: garantir `firmware/build/`, `node_modules`, `temp_*` fora do git;
   mover `firmware_legacy_reference/` para fora do working tree (submódulo,
   branch órfã ou zip).

**Endurecer no curto prazo (semanas):**

5. Extrair o áudio de `app_main` para um módulo (`board/` ou `services/audio`),
   com o invariante "display lock = I2C mutex" encapsulado atrás de uma função
   nomeada (`board::i2c_lock()`), mesmo que a implementação seja o mesmo lock.
6. Resolver a contradição coredump-vs-prod em ADR (ex.: coredump on + partição
   dedicada + wipe em provisioning, ou triagem alternativa).
7. Emitir `ResourceWarning` de verdade (watermark de heap interno no tick de
   SystemService) e persistir contadores mínimos de falha por domínio.
8. Escrever o documento-orçamento de recursos (SRAM interna, banda MSPI,
   regras de alocação por tipo de dado) consolidando o que hoje está em
   comentários — é o contrato que protege as futuras features do hardware.

**Reconsiderar arquiteturalmente (antes da Fase 5):**

9. **Padrão de tela/tile com registro** — cada tela declara `{ScreenId, build,
   update, eventos_que_me_invalidam}` e o shell itera o registro. Elimina o
   mapa N×M de flags em `app_main` e o boilerplate por tela. Este é o refactor
   que decide se a Fase 5 constrói ativo ou dívida.
10. Introduzir interfaces de provider (IMarketProvider etc.) **no momento em
    que houver o primeiro teste de service ou a primeira troca de API** — não
    antes por dogma, mas registre em ADR que a doc atual promete o que o código
    não tem.
11. Decidir OTA (partições A/B + tamanho de app + estratégia de rollback) antes
    de qualquer unidade com Flash Encryption RELEASE. Reavaliar anti-rollback
    até lá.

**Pode continuar como está:**

12. EventBus síncrono int32, StateStore por snapshot, loop de 200 ms, cache
    binário versionado, NetworkWorker serializado — tudo dimensionado certo
    para o produto real. Não sofistique.

**Simplificar ou remover:**

13. Domínios mortos do `DataDomain`/políticas (Photos, Notifications, Calendar
    sem fetcher) — ou marcar explicitamente como reserva.
14. Uma das duas camadas de coalescing (a do `UiDispatcher` deveria ser a
    única dona; os flags de `app_main` somem naturalmente com o item 9).
15. `shared/` — congelar até existir a segunda ponta do contrato.

## 13. Julgamento final

**Esta arquitetura merece continuar? Sim, sem hesitação.** O fluxo de
autoridade está certo, o modelo de concorrência foi *simplificado* sob pressão
(N tasks → 1 worker) em vez de complicado — o sinal mais confiável de bom
julgamento arquitetural — e as decisões difíceis estão escritas com números.
Não há aqui nenhuma fundação que exija demolição.

**A base atual suporta um produto premium? Ainda não.** Suporta um excelente
dispositivo pessoal. A distância até "premium" não está na estética nem no
fluxo de dados — está em: atualização em campo (OTA), prova de resistência
(soak + telemetria contínua), profundidade de teste nas bordas (parsing,
memória), uma camada de UI que cresça por composição em vez de repetição, e um
repositório cuja documentação seja confiável por construção.

**Decisões a revisitar antes de investir mais nesta base, em ordem:**
(1) verdade documental única — CLAUDE.md/ROADMAP/ARCHITECTURE convergidos;
(2) padrão de tela registrável antes da Fase 5;
(3) OTA antes de qualquer provisionamento PROD;
(4) orçamento explícito de SRAM/barramento como contrato de plataforma.

O projeto tem o ativo mais difícil de comprar — disciplina de decisão e
conhecimento íntimo do hardware. O que falta é o mais fácil de planejar:
completude, teste e a coragem de fazer o refactor de UI antes de escalar as
telas. Se a ordem acima for respeitada, esta base sustenta o produto que os
docs descrevem.
