# NovaPainel - Roadmap

> Estado revisado em 2026-06-26.
>
> Confirmado: o que esta realmente fechado no projeto ate aqui e a **Fase 0**
> (hardware/risk gates) e a **trilha H** de melhorias tecnicas/documentais
> derivadas desse trabalho. No `firmware/` reescrito, o que existe hoje e um
> novo baseline com algumas entregas de UI e da espinha dorsal do core, mas
> isso nao significa que as antigas fases funcionais do firmware ja tenham sido
> reentregues no tree novo.
>
> O baseline ativo agora e o `firmware/` reescrito. O tree antigo foi
> preservado em `firmware_legacy_reference/` apenas como referencia de porta e
> consulta. O roadmap abaixo parte dessa realidade.

## Diagnostico atual

Hoje o firmware novo entrega uma base valida, mas ainda enxuta:

- boot do projeto ESP-IDF para `esp32p4`
- inicializacao do display via BSP oficial da Waveshare
- shell LVGL novo com rail, topbar, dots e menu lateral
- telas `Boot`, `Setup`, `Home` e `Agenda` ja recriadas visualmente
- placeholders para `Market`, `Casa`, `Focus`, `Photo`, `Rotinas`,
  `Configuracoes` e `Sistema`
- `EventBus`, `StateStore`, `UiDispatcher`, `ServiceManager` e
  `RequestOrchestrator` presentes no novo tree
- telemetria simples de heap/uptime em `app_main.cpp`

Ao mesmo tempo, o reboot ainda nao religou:

- componente `board/` proprio do firmware
- `services/`, `providers/` e `cache/`
- onboarding funcional com NVS/Wi-Fi/NTP
- dados reais de clima/mercado
- tela de sistema real
- persistencia, observabilidade e hardening anteriores
- testes host/native e validacao de build alinhados ao novo tree

## O que esta concluido de verdade

```text
Fase 0 - validacao de hardware, BSP, risco de plataforma e gates reais   [feito]
Trilha H - consolidacao tecnica/documental pos-Fase 0                    [feito]
Reboot UI - shell novo + algumas telas portadas visualmente              [parcial]
```

Leitura correta:

- Fase 0 e H ficam como patrimonio tecnico do projeto.
- O `firmware/` novo reaproveita esse aprendizado, mas ainda nao reentregou
  as antigas fases de produto no codigo ativo.
- As entregas de UI do reboot contam como progresso real, mas ainda sao base
  visual/estrutural, nao fechamento de fases funcionais como Setup, cache,
  providers, sistema e operacao.

## Regras para esta fase do projeto

- O `firmware/` novo e a unica base de evolucao.
- `firmware_legacy_reference/` serve para port seletivo, nunca para voltar o tree inteiro.
- Cada feature volta apenas quando houver:
  - contrato de estado claro em `models/`
  - wiring pelo `StateStore` e `EventBus`
  - tela consumindo estado, sem request direto
  - validacao no host e build IDF quando aplicavel
- A prioridade e reconstruir fundacao e fluxo real sobre o baseline novo,
  aproveitando Fase 0 e H como contexto, antes de contar fases funcionais como
  reentregues.

## Trilha R - Reboot do firmware

```text
R0 - Alinhar documentacao com o baseline novo                          [em andamento]
R1 - Consolidar shell/UI baseline e mapa de telas                      [pendente]
R2 - Reintroduzir board/service wiring minimo                          [pendente]
R3 - Reativar setup, Wi-Fi, NTP e preferencias                         [pendente]
R4 - Reativar providers, cache e estado real da Home                  [pendente]
R5 - Portar telas restantes com dados reais                           [pendente]
R6 - Recuperar observabilidade, resiliencia e operacao                [pendente]
R7 - Voltar ao hardening de release e v1.0                            [futuro]
```

## Fases propostas

### Fase A - Baseline honesto e reprodutivel

Objetivo: fechar a divergencia entre codigo e documentacao.

Escopo:

- atualizar `ARCHITECTURE.md`, `PLANEJAMENTO.md` e docs operacionais para o reboot
- registrar o que ja foi portado no shell novo
- documentar quais pecas do legado ainda sao referencia
- limpar claims antigas de fases fechadas que nao existem no `firmware/` atual

Criterio de saida:

- documentacao descreve exatamente o que o `firmware/` novo faz hoje
- backlog de portas do legado vira lista explicita, nao conhecimento tacito
- Fase 0 e H permanecem reconhecidas como concluidas sem inflar o estado do
  `firmware/` novo

### Fase B - Fundacao do firmware novo

Objetivo: tornar o tree novo um ponto estavel para crescer sem gambiarra.

Escopo:

- revisar `app_main.cpp` para sair de wiring temporario
- decidir se `board/` volta ja nesta fase ou fica encapsulado no `main/`
- religar `ServiceManager` no loop principal
- revisar contratos de `AppState`, `EventType` e `UiDispatcher`
- recuperar validacao rapida no host para `core/`, `models/` e `main/` onde possivel

Criterio de saida:

- loop principal nao depende de wiring descartavel
- core e modelos do reboot compoem uma base coerente para religar services

### Fase C - Setup e conectividade

Objetivo: voltar a sair do modo demonstracao visual para fluxo funcional.

Escopo:

- recriar `SetupService`
- persistir nome, timezone e formato de hora em NVS
- reativar scan Wi-Fi, conexao e estados de onboarding
- religar NTP e estado de relogio sincronizado
- decidir o minimo de `SystemStatus` que volta junto

Criterio de saida:

- painel consegue sair do `Setup` para `Home` com preferencia salva
- relogio deixa de ser mock visual do layout

### Fase D - Dados reais e cache

Objetivo: reintroduzir utilidade real sem perder o offline-first.

Escopo:

- recriar `WeatherService`, `MarketService` e providers minimos
- religar `RequestOrchestrator` ao fluxo de request real
- reintroduzir `CacheStore` ou substituto equivalente
- semear `Home` por cache no boot
- restaurar stale/source/last_update no estado

Criterio de saida:

- Home mostra clima e mercado reais
- sem rede, o painel continua util com cache explicito

### Fase E - Port das telas por prioridade

Objetivo: expandir interface sem espalhar placeholders eternos.

Ordem recomendada:

1. `System`
2. `Settings`
3. `Market`
4. `Devices`
5. `Routines`
6. `Focus` / `PhotoFrame`

Regra:

- cada tela so sai do placeholder quando tiver estado e navegacao reais
- se a tela depender de provider/servico ausente, ela continua placeholder por decisao explicita

Criterio de saida:

- placeholders restantes sao poucos e justificados
- mapa visual do export LVGL e do firmware ficam sincronizados

### Fase F - Observabilidade e resiliencia

Objetivo: recuperar as partes "de produto real" no tree novo, sem fingir que ja voltaram.

Escopo:

- reset reason e reboot count
- logs de campo e operacao
- cache auto-recuperavel
- circuit breaker/backoff confirmados no fluxo real
- coredump, release/rollback e soak voltam a ser verdade quando o codigo voltar

Criterio de saida:

- docs operacionais voltam a refletir implementacoes reais do tree novo
- falha de rede ou reboot nao vira perda total de contexto

### Fase G - Hardening e v1.0

Objetivo: so endurecer o que ja existir de verdade no reboot.

Escopo futuro:

- seguranca PROD
- performance sob carga
- ajustes de PSRAM/render
- modo noite, timer, album, perfis simples
- sensores e automacao seguem pos-v1 inicial

## Ordem pratica das proximas entregas

```text
1. Documentacao do reboot
2. Wiring minimo do core no firmware novo
3. SetupService + Wi-Fi + NTP
4. Clock/Home alimentados por estado real
5. Weather/Market + cache
6. System/Settings
7. Resiliencia e operacao
8. v1.0 de features
```

## O que nao fazer agora

- nao tentar "reimportar tudo" do legado num unico commit
- nao tratar docs antigas como prova de que a feature ja voltou
- nao retomar secure boot, release ou soak como prioridade antes de setup e dados reais
- nao expandir novas features de produto com placeholders ainda dominando o tree novo

## Resumo executivo

O reboot foi a decisao certa: o `firmware/` novo ja tem base visual melhor,
estrutura mais limpa e um caminho de evolucao mais seguro. O proximo roadmap
precisa assumir isso com honestidade: nao estamos fechando fases 12+, estamos
reconstruindo de forma controlada as fases funcionais essenciais no baseline
novo para depois voltar ao hardening e ao v1.0.
