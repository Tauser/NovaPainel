# NovaPainel - Arquitetura

> Arquitetura alvo do produto e baseline de engenharia do firmware.
>
> Este documento descreve o desenho que deve guiar o projeto, separando
> claramente a arquitetura desejada do estado parcial do reboot atual.

## 1. Objetivos de arquitetura

A arquitetura do NovaPainel existe para sustentar:

- offline-first real
- baixo acoplamento entre UI, dominio, hardware e rede
- crescimento por modulos
- debug e operacao de campo viaveis
- manutencao simples para um firmware de vida longa

## 2. Contexto do sistema

```text
Usuario
  ->
NovaPainel Firmware (principal)
  -> display/touch/hardware local
  -> NVS + cache local
  -> APIs externas opcionais
  -> server opcional futuro
```

Regras de contexto:

- o firmware continua util sem `server/`
- internet melhora o produto, mas nao define a utilidade basica
- qualquer integracao futura deve preservar esse contrato

## 3. Fluxo principal de dados

```text
Provider -> Service -> StateStore -> EventBus -> UiDispatcher -> lvgl_task -> UI
```

Sem excecoes:

- `Provider` fala com API, filesystem ou hardware especializado
- `Service` aplica regras de negocio
- `StateStore` guarda o estado canonico
- `EventBus` propaga mudancas
- `UiDispatcher` traduz eventos relevantes para a UI
- apenas a `lvgl_task` do BSP toca objetos LVGL

## 4. Camadas do firmware

```text
main/
  wiring do runtime e loop principal

components/
  core/
  models/
  board/
  providers/
  services/
  ui/
  utils/
```

### 4.1 `core/`

Responsabilidades:

- `EventBus`
- `StateStore`
- `UiDispatcher`
- `RequestOrchestrator`
- `ServiceManager`

Regras:

- nao conhece detalhes concretos de API externa
- nao conhece widgets LVGL
- deve continuar host-checkavel sempre que possivel

### 4.2 `models/`

Responsabilidades:

- structs e enums do estado global
- contratos simples de dados
- nada de IO, rede ou acoplamento com ESP-IDF quando evitavel

### 4.3 `board/`

Responsabilidades:

- encapsular hardware e primitives de lock da UI
- abstrair BSP e dependencias fisicas
- permitir portar o firmware sem contaminar o resto do sistema

### 4.4 `providers/`

Responsabilidades:

- adaptadores de APIs ou fontes externas
- traducao de payload externo para modelos internos

Regras:

- provider nao conhece UI
- provider nao escreve no `StateStore`
- provider retorna dado/status; service decide o que fazer

### 4.5 `services/`

Responsabilidades:

- orquestrar dominios como clock, setup, clima, mercado, sistema e notificacoes
- decidir quando pedir dados
- atualizar o `StateStore`

Regras:

- service nao toca widget LVGL
- service nao chama provider fora do `RequestOrchestrator`
- service pode depender de interfaces, nunca de adaptadores concretos por acoplamento forte

### 4.6 `ui/`

Responsabilidades:

- shell visual
- telas
- componentes de apresentacao

Regras:

- UI le estado
- UI publica intencao
- UI nao persiste diretamente
- UI nao chama Wi-Fi, HTTP, filesystem ou NVS

## 5. Modelo de runtime

### 5.1 Loop principal

O `main/` deve:

- inicializar hardware baseline
- montar dependencias do app
- iniciar services
- alimentar `tick()` dos services
- drenar eventos de UI sob o lock apropriado

### 5.2 Concorrencia

Regras obrigatorias:

- `EventBus` pode ser sincrono para eventos internos leves
- render nunca pode bloquear por causa de service lento
- qualquer acesso a LVGL fora da task do BSP exige lock
- operacoes longas devem ser deferidas para fora do callback de toque/UI

### 5.3 Responsabilidade de tempo

- `ClockService` e o dono do estado de relogio
- `SetupService` e o dono do onboarding e conectividade
- `SystemService` e o dono do diagnostico operacional
- `MarketService` e `WeatherService` sao donos dos dominios de dados reais

## 6. Estado e contratos

`AppState` e a fonte unica de verdade do firmware.

Categorias de estado:

- navegacao e tela atual
- clock
- mercado
- clima
- sistema
- preferencias do usuario
- onboarding e conectividade

Regras:

- atualizacoes devem ser pequenas, previsiveis e observaveis
- dados stale precisam ser representados no modelo
- `source`, `valid`, `last_update_ms` e flags equivalentes devem existir onde
  a UX precise distinguir dado real, cache ou mock

## 7. Persistencia

### 7.1 NVS

Uso:

- Wi-Fi
- preferencias do usuario
- flags operacionais pequenas
- diagnostico simples persistido

Regras:

- versionar schema
- prever migracao
- nunca logar segredos

### 7.2 Cache local

Uso:

- clima
- mercado
- outros dados de leitura com fallback offline

Regras:

- `LittleFS`
- escrita atomica
- auto-recuperacao de dados temporarios/corrompidos
- incompatibilidade de schema nao pode brickar o boot

## 8. Requests e resiliencia

`RequestOrchestrator` e a porta unica de saida.

Ele deve centralizar:

- enable/disable por dominio
- prioridade
- intervalo minimo
- rate limit
- circuit breaker
- backoff exponencial com jitter

Contrato de UX:

- se a rede falhar, a UI continua operavel
- se a API falhar, o app mostra cache/stale
- se nao houver dado algum, o app comunica isso de forma clara

## 9. UI e design system

### 9.1 Fonte visual

O export em `docs/design/lvgl_export_reference/` continua sendo a referencia
visual para o firmware.

### 9.2 Organizacao visual

O shell atual do reboot estabelece:

- rail lateral
- topbar
- area de conteudo
- dots de navegacao
- telas independentes por `ScreenId`

### 9.3 Regras de implementacao

- tela nova deve nascer a partir do export visual
- placeholder e aceitavel somente quando documentado no roadmap
- tela nao sai do placeholder sem estado e navegacao coerentes

## 10. Operacao e release

A arquitetura precisa suportar:

- reset reason
- reboot count
- logs uteis de campo
- coredump e triagem
- release com rollback consciente
- diferenca clara entre DEV e PROD

Isso deve permanecer em documentos dedicados:

- `SECURITY-OPERATIONS.md`
- `FIELD-OPERATIONS.md`
- `RELEASE-ROLLBACK.md`
- `SOAK-VALIDATION.md`

## 11. Testabilidade

Piramide recomendada:

- testes host/unit para `core/`, `models/` e logica pura
- validacao de build IDF para integracao de firmware
- validacao em hardware para board/BSP/rede/touch/display
- soak e operacao para cenarios de campo

Regras:

- preferir logica desacoplada de ESP-IDF
- contratos devem ser testaveis sem hardware sempre que possivel
- comportamento de degradacao deve ser testado explicitamente

## 12. Extensibilidade futura

A arquitetura precisa comportar sem ruptura:

- sensores locais
- automacao residencial
- album/fotoframe
- player/feedback sonoro
- voz
- NoiseBot
- server opcional

Regra:

- extensao nova entra por modulo, nao por gambiarras cruzando camadas

## 13. Estado atual do reboot

O `firmware/` novo hoje implementa parcialmente esta arquitetura:

- `core/`, `models/` e `ui/` estao reerguidos
- `ServiceManager`, `BootstrapService`, `SetupService`, `SystemService` e `ClockService`
  ja voltaram ao runtime
- shell visual novo esta funcional
- `Boot`, `Setup`, `Home` e `Agenda` ja foram portadas visualmente
- varias telas seguem placeholder
- `board/`, `providers/`, `services/` e `cache/` ainda precisam voltar como
  implementacao funcional do tree ativo

Leitura correta:

- a arquitetura alvo nao mudou
- o estado implementado ainda e parcial
- a evolucao deve religar modulos sem quebrar os limites definidos aqui
