# NovaPainel - Roadmap Consolidado

> Roadmap oficial do projeto.
>
> Este documento preserva toda a base do planejamento inicial, reconhece o que
> ja foi consolidado em hardware/documentacao e organiza a reconstrucao do
> `firmware/` novo em fases completas, escalaveis e auditaveis.

## 1. Leitura correta do estado atual

Patrimonio concluido:

```text
Fase 0 - hardware, BSP, conectividade-base e gates de risco             [feito]
Trilha H - consolidacao tecnica/documental pos-Fase 0                   [feito]
Reboot do firmware - shell novo + parte da UI + base de core            [parcial]
```

Isso significa:

- a fundacao de hardware ja foi provada
- a direcao arquitetural do projeto continua valida
- o `firmware/` ativo ainda precisa reentregar as fases funcionais no tree novo

## 2. Principios de execucao

- nenhuma fase conta como entregue sem criterio claro de saida
- `firmware/` novo e a unica base de evolucao
- `firmware_legacy_reference/` e somente referencia para port seletivo
- cada retomada funcional precisa religar codigo, docs e validacao
- otimizar, endurecer ou expandir antes da base funcional voltar e perda de foco

## 3. Visao completa de fases

```text
Fase 0   - Hardware e risco de plataforma                               [feito]
Fase H   - Consolidacao tecnica/documental pos-Fase 0                   [feito]
Fase 1   - Consolidacao documental e baseline canonico                  [feito]
Fase 2   - Estabilizacao do reboot do firmware                          [pendente]
Fase 3   - Setup, conectividade e tempo                                 [pendente]
Fase 4   - Dados reais e cache offline                                  [feito]
Fase 5   - Telas funcionais centrais do MVP                             [pendente]
Fase 6   - Observabilidade e resiliencia de operacao                    [pendente]
Fase 7   - Hardening de release e seguranca PROD                        [pendente]
Fase 8   - v1.0 Painel Pessoal                                          [futuro]
Fase 9   - Sensores e automacao local                                   [futuro]
Fase 10  - Midia, ambient intelligence e experiencias avancadas         [futuro]
Fase 11  - Ecossistema opcional: server, voz, NoiseBot e integracoes    [futuro]
```

## 4. Fase 0 - Hardware e risco de plataforma

Status: **feito**

Cobertura:

- validacao da placa Waveshare ESP32-P4-WIFI6-Touch-LCD-7B
- display, touch, backlight e BSP oficial
- PSRAM
- ESP-Hosted / SDIO P4 <-> C6
- Wi-Fi real
- HTTPS
- NTP
- SD
- coexistencia display + touch + rede + PSRAM
- RTC e criterio de clock offline
- brownout e termica

Fonte de verdade:

- [HARDWARE.md](D:\Projetos\NovaPanel\docs\HARDWARE.md)
- [FASE0-CHECKLIST.md](D:\Projetos\NovaPanel\docs\FASE0-CHECKLIST.md)

## 5. Fase H - Consolidacao tecnica/documental pos-Fase 0

Status: **feito**

Cobertura patrimonial do projeto:

- seguranca operacional DEV/PROD
- field operations
- release/rollback
- soak validation
- consolidacao de achados tecnicos da plataforma

Importante:

- Fase H continua valida como patrimonio tecnico
- isso nao significa que todas essas capacidades ja estao reentregues no
  `firmware/` novo

## 6. Fase 1 - Consolidacao documental e baseline canonico

Status: **feito**

Objetivo:

- limpar a documentacao canônica
- preservar o planejamento inicial completo
- separar claramente visao de produto, patrimonio tecnico e estado atual do reboot

Escopo:

- roadmap novo e completo
- planejamento alinhado ao estado real
- arquitetura alvo explicitada
- README e indice de docs coerentes
- backup dos documentos antigos

Criterio de saida:

- um leitor novo entende o projeto sem cruzar verdades conflitantes
- a documentacao deixa claro o que e visao, o que e concluido e o que ainda e reboot parcial

Resultado:

- concluida em 2026-06-26 com consolidacao dos documentos centrais, README,
  roadmap, arquitetura e playbooks operacionais, preservando backup dos
  arquivos anteriores

## 7. Fase 2 - Estabilizacao do reboot do firmware

Objetivo:

- transformar o reboot em uma base limpa, previsivel e escalavel

Escopo:

- estabilizar `main/`
- religar `ServiceManager` ao runtime
- reintroduzir os servicos basicos de bootstrap, sistema e clock
- decidir e reinstalar a camada `board/` no tree novo
- revisar contratos de `AppState`, `EventType` e `UiDispatcher`
- garantir host-check e build IDF coerentes com o baseline
- consolidar shell e navegacao

Criterio de saida:

- o `firmware/` novo sustenta crescimento sem wiring temporario
- o core fica novamente pronto para receber servicos reais

## 8. Fase 3 - Setup, conectividade e tempo

Objetivo:

- sair do baseline visual e voltar a um fluxo funcional minimo do usuario

Escopo:

- `SetupService`
- onboarding funcional
- scan Wi-Fi
- conexao Wi-Fi
- persistencia de nome, timezone e formato de hora
- NTP
- integracao entre RTC e estado de clock
- configuracoes basicas reusaveis fora do primeiro boot

Criterio de saida:

- usuario conclui onboarding real
- o painel sai de `Setup` para `Home` com estado salvo
- o relogio para de ser apenas conteudo visual estatico

## 9. Fase 4 - Dados reais e cache offline

Status: **feito**

Objetivo:

- restaurar utilidade concreta do painel sem quebrar o offline-first

Escopo:

- `WeatherService`
- `MarketService`
- `ForexProvider`
- `OpenMeteoProvider`
- `CoinGeckoProvider`
- `CacheStore` ou equivalente
- stale/source/last_update no fluxo real
- `RequestOrchestrator` ligado ao caminho de requests

Criterio de saida:

- Home mostra dados reais
- sem internet, o painel continua util via cache
- o comportamento degradado fica claro para o usuario

Resultado:

- concluida no `firmware/` ativo com `WeatherService`, `MarketService`,
  `ForexService`, `OpenMeteoProvider`, `CoinGeckoProvider`, `ForexProvider`,
  `CacheStore` em LittleFS e `RequestOrchestrator` no caminho de requests
- Home consome estado real/cache de clima, BTC e USD/BRL sem requests diretos,
  com `live/cache/mock`, `stale/source/last_update` e degradacao visivel
- cache dividido por dominio (`market`, `forex`, `weather`) para evitar que
  atualizacoes independentes se sobrescrevam
- validada com `bash tools/scripts/host_check.sh --app` e build ESP-IDF
  `idf.py build` em `firmware/` no target `esp32p4`

Checkpoint de campo:

- flash/monitor em hardware real deve ser usado como evidencia operacional
  antes de encerrar uma rodada de release, mas nao bloqueia a conclusao
  funcional da fase no tree ativo

## 10. Fase 5 - Telas funcionais centrais do MVP

Objetivo:

- fechar o miolo do produto com UX e navegacao reais

Escopo:

- `Home`
- `Agenda`
- `Market`
- `Settings`
- `System`
- placeholders restantes apenas quando dependencias nao existirem ainda

Prioridade recomendada:

1. `System`
2. `Settings`
3. `Market`
4. consolidacao da `Home`
5. consolidacao da `Agenda`
6. `Devices`, `Routines`, `Focus`, `PhotoFrame`

Criterio de saida:

- o MVP deixa de parecer um shell demonstrativo
- telas centrais consomem estado real e nao conteudo estatico

## 11. Fase 6 - Observabilidade e resiliencia de operacao

Objetivo:

- recuperar no tree novo as garantias operacionais do produto

Escopo:

- reset reason
- reboot count
- status operacional por dominio
- logs uteis de campo
- coredump no contexto certo
- cache auto-recuperavel
- circuit breaker e backoff em operacao real
- testes de degradação e reconexao

Criterio de saida:

- reboot, falha de rede e API indisponivel nao viram caixa-preta
- o time consegue operar e depurar o dispositivo em campo

## 12. Fase 7 - Hardening de release e seguranca PROD

Objetivo:

- transformar o baseline funcional em produto liberavel

Escopo:

- DEV vs PROD formalizado
- `NVS Encryption`
- `Flash Encryption`
- `Secure Boot v2`
- build e provisioning controlados
- rollback consciente
- soak de release

Criterio de saida:

- existe caminho repetivel e seguro para release
- segredos, logs e rollback seguem politica clara

## 13. Fase 8 - v1.0 Painel Pessoal

Objetivo:

- entregar o primeiro pacote forte de experiencia de produto

Escopo preservado do planejamento inicial:

- home adaptativa simples
- modo noite
- relogio premium
- alertas simples de preco
- linha do tempo do dia
- album inteligente
- timer/Pomodoro
- feedback sonoro
- perfis simples
- rotinas locais basicas

## 14. Fase 9 - Sensores e automacao local

Objetivo:

- expandir o painel para contexto ambiental e automacao residencial

Escopo:

- presenca
- luz ambiente
- temperatura/umidade/ar
- Sonoff local
- cenas rapidas reais
- estado de dispositivos
- planta da casa, se ainda fizer sentido no produto

## 15. Fase 10 - Midia e experiencias avancadas

Objetivo:

- adicionar camadas mais sofisticadas de experiencia sem comprometer o core

Escopo:

- photoframe/album robusto
- player
- recursos de foco
- experiencias ambient intelligence locais
- futuras otimizacoes de render e memoria

## 16. Fase 11 - Ecossistema opcional

Objetivo:

- integrar capacidades futuras sem quebrar o contrato offline-first

Escopo:

- `server/` opcional
- bridge de servicos locais
- voz
- integracao com NoiseBot
- monitoramento, camera ou interfone se mantiverem valor de produto

Regra:

- nenhuma dessas integracoes pode virar dependencia estrutural do firmware

## 17. Itens explicitamente fora do escopo atual

- Samsung TV
- automacao fisica pesada antes do baseline funcional
- WebSocket/candles como requisito do MVP
- crescimento do `server/` antes do firmware justificar isso

## 18. Ordem pratica recomendada a partir de agora

```text
1. Fechar Fase 1 documental
2. Executar Fase 2 e estabilizar o reboot
3. Reentregar Fase 3 com Setup/Wi-Fi/NTP/Clock
4. Reentregar Fase 4 com dados reais e cache
5. Reentregar Fase 5 com telas centrais do MVP
6. Reentregar Fase 6 com observabilidade/resiliencia
7. Avancar para Fase 7 e preparar release
8. Abrir v1.0 em diante
```

## 19. Definicao de pronto por fase

Uma fase so conta como concluida quando houver, no minimo:

- codigo funcional no `firmware/` ativo
- documentacao canonica atualizada
- criterio de validacao executado
- estado de backlog seguinte claro

## 20. Backup da documentacao anterior

Snapshot preservado em:

- `docs/_backup/2026-06-26-pre-consolidation/`
