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
