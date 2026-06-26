# NovaPainel - Planejamento do Projeto

> Documento canonico do produto e da estrategia tecnica.
>
> Este arquivo preserva a base do planejamento inicial do NovaPainel, mas
> atualiza a leitura de status para o baseline real do repositorio em
> 2026-06-26.

## 1. Visao do produto

O **NovaPainel** e um smart display local, offline-first e orientado a uso
diario, baseado em **ESP32-P4 + ESP32-C6**, com firmware em **ESP-IDF + LVGL**,
arquitetura modular, cache local e integracoes futuras com automacao,
sensores, voz e ecossistema NoiseBot.

Regra principal do produto:

```text
O painel precisa continuar util, responsivo, bonito e previsivel
mesmo quando internet, API, storage, memoria ou algum servico falhar.
```

## 2. Objetivos de engenharia

O projeto deve ser:

- escalavel em escopo, sem colapsar a manutencao
- robusto contra falhas de rede, reboot e dados parciais
- confiavel em operacao continua
- facil de testar, evoluir e portar
- sustentado por componentes e limites bem definidos

Principios obrigatorios:

- `offline-first`: o firmware nunca depende do `server/`
- `state-driven UI`: a UI le do estado; nao faz request direto
- `ports and adapters`: hardware e APIs externas entram por interfaces
- `single source of truth`: `StateStore` e o estado unico da aplicacao
- `safe concurrency`: somente a `lvgl_task` do BSP toca objetos LVGL
- `graceful degradation`: falha externa vira cache/stale, nao travamento
- `small recoverable steps`: fases, commits e migracoes devem ser reversiveis

## 3. Escopo funcional do produto

### 3.1 Mantido no projeto

- Home simples e progressivamente adaptativa
- data e hora
- Wi-Fi com onboarding e reconfiguracao posterior
- clima basico
- BTC e USD/BRL
- cache local com stale explicito
- tela de sistema/status
- configuracoes basicas do usuario e conectividade
- observabilidade minima de campo
- arquitetura modular para futuras extensoes
- server opcional no futuro, sem dependencia do firmware

### 3.2 Removido do projeto

- controle de TV Samsung
- Wake-on-LAN, SmartThings, IR para TV e tokens Samsung

Motivo: risco alto demais para o valor entregue no MVP inicial.

### 3.3 Futuro planejado

- Sonoff LAN, Tasmota, ESPHome ou MQTT
- cenas rapidas e automacao local
- planta da casa e estado de dispositivos
- sensores de presenca, luz, temperatura, umidade e qualidade do ar
- timer, Pomodoro, modo noite, album e perfis simples
- voz, monitoramento, camera, interfone
- integracao opcional com NoiseBot

## 4. Escopo tecnico base

### 4.1 Hardware alvo

- placa principal: Waveshare ESP32-P4-WIFI6-Touch-LCD-7B
- CPU principal: ESP32-P4
- conectividade: ESP32-C6 via ESP-Hosted/SDIO
- display: 1024x600 via BSP oficial Waveshare
- touch: GT911
- armazenamento/persistencia: NVS + filesystem local

### 4.2 Requisitos tecnicos obrigatorios

- boot estavel e previsivel
- coexistencia segura entre display, touch, PSRAM e rede
- cache sobrevivendo a reboot e falta de internet
- UI sem travar por causa de request ou service lento
- arquitetura testavel no host sempre que possivel
- docs, contratos e roadmap andando junto com o codigo

## 5. Arquitetura alvo

Fluxo principal do sistema:

```text
Provider -> Service -> StateStore -> EventBus -> UiDispatcher -> lvgl_task -> UI
```

Camadas esperadas:

- `board/`: HAL do hardware e pontos de sincronizacao da UI
- `providers/`: adaptadores de APIs externas
- `services/`: logica de dominio
- `core/`: estado, eventos, orchestrator, wiring de runtime
- `models/`: contratos de estado e dados
- `ui/`: shell, telas e componentes visuais
- `shared/`: contratos compartilhados com um server futuro

Regras de arquitetura:

- UI nao persiste dados e nao acessa hardware/rede diretamente
- services nao renderizam UI
- providers nao conhecem UI nem tela
- toda mutacao de estado passa por `StateStore`
- todo request externo passa por `RequestOrchestrator`
- toda interacao com LVGL fora do BSP exige o lock apropriado

## 6. Estrategia de dados

### 6.1 Mercado

- `CoinGecko` REST para BTC no MVP
- `ForexProvider` dedicado para USD/BRL
- polling conservador e budget controlado
- cache obrigatorio
- WebSocket e candles ficam para evolucao futura

### 6.2 Clima

- `Open-Meteo` no MVP
- sem chave externa como dependencia inicial
- localizacao configuravel depois do baseline funcional

### 6.3 Persistencia

- `NVS` para preferencias e conectividade
- `LittleFS` para cache local
- escrita atomica
- versionamento e migracao de schema

## 7. Requisitos nao funcionais

### 7.1 Escalabilidade

- crescer por modulos independentes
- permitir adicionar services/providers sem refatorar o core
- preservar contratos claros entre camadas

### 7.2 Robustez e confiabilidade

- circuit breaker e backoff por dominio
- cache como fallback normal de operacao
- reset reason, reboot count e diagnostico de campo
- release com rollback planejado

### 7.3 Manutenibilidade

- baixo acoplamento entre UI, dominio e IO
- interfaces pequenas e explicitas
- componentes host-checkaveis
- documentacao canonica curta, direta e coerente
- ADRs para decisoes que mudem arquitetura, seguranca ou release

### 7.4 Seguranca

- segredos fora de logs
- layout e particoes pensando em producao
- `NVS Encryption`, `Flash Encryption` e `Secure Boot v2` para release

## 8. Melhor stack e praticas para este projeto

Direcao tecnologica recomendada:

- **ESP-IDF 5.5.x** como base principal
- **BSP oficial Waveshare** antes de HAL custom sem necessidade
- **LVGL 9.x** como stack de UI
- **C++ moderno e simples** no firmware, privilegiando RAII, tipos fortes e
  interfaces pequenas
- **NVS + LittleFS** para persistencia
- **contratos em `shared/`** para qualquer evolucao futura de bridge/server

Praticas obrigatorias:

- usar bibliotecas oficiais ou primarias sempre que possivel
- evitar reimplementar stack de hardware onde o BSP oficial resolve
- preferir composicao e injecao de dependencia a singletons globais
- validar no host antes do hardware quando possivel
- manter backlog por fases com criterio claro de saida

## 9. Estado real do projeto hoje

Patrimonio tecnico consolidado:

- **Fase 0 concluida**: hardware, BSP e gates de risco validados
- **trilha H concluida**: consolidacao tecnica e documental pos-Fase 0

Estado do `firmware/` ativo:

- baseline novo com BSP, shell LVGL, `Boot`, `Setup`, `Home` e `Agenda`
  portadas visualmente
- `core/`, `models/` e `ui/` reerguidos
- reboot ainda sem reintroduzir todos os services, providers, cache,
  onboarding funcional, observabilidade e release path no tree novo

Conclusao correta:

- a visao inicial do projeto segue valida
- a fundacao de hardware esta provada
- o firmware ativo esta em fase de reconstrucao funcional controlada

## 10. O que o MVP precisa conter

Para este projeto ser considerado MVP funcional no baseline novo, ele precisa
entregar de ponta a ponta:

- boot estavel
- display e touch reais
- onboarding funcional
- Wi-Fi e NTP
- clock coerente com RTC/NTP
- Home com dados reais de clima, BTC e USD/BRL
- cache local com stale explicito
- tela de sistema/status
- configuracoes basicas editaveis
- observabilidade minima
- resiliencia de request

## 11. Roadmap de implementacao

A ordem oficial das fases esta em [ROADMAP.md](D:\Projetos\NovaPanel\docs\ROADMAP.md).

Resumo:

```text
1. preservar Fase 0/H como base do projeto
2. consolidar documentacao canonica
3. estabilizar o firmware novo
4. religar conectividade, tempo e preferencias
5. religar dados reais e cache
6. religar telas centrais e operacao
7. endurecer release
8. expandir para v1.0, sensores, automacao e integracoes futuras
```

## 12. Fonte de verdade documental

Documentos canonicos:

- [README.md](D:\Projetos\NovaPanel\README.md)
- [docs/README.md](D:\Projetos\NovaPanel\docs\README.md)
- [docs/PLANEJAMENTO.md](D:\Projetos\NovaPanel\docs\PLANEJAMENTO.md)
- [docs/ARCHITECTURE.md](D:\Projetos\NovaPanel\docs\ARCHITECTURE.md)
- [docs/HARDWARE.md](D:\Projetos\NovaPanel\docs\HARDWARE.md)
- [docs/ROADMAP.md](D:\Projetos\NovaPanel\docs\ROADMAP.md)
- [docs/DECISIONS.md](D:\Projetos\NovaPanel\docs\DECISIONS.md)

Backup dos documentos anteriores:

- `docs/_backup/2026-06-26-pre-consolidation/`
