# NovaPainel - Planejamento Técnico (consolidado, base v3)

> Documento canônico. Substitui rascunhos anteriores (v1/v2).

## 1. Visão do Produto

O **NovaPainel** é um smart display local/offline-first baseado em ESP32-P4, com
foco em uso diário, informações pessoais, dados de mercado, agenda, cache local
e tela de sistema, com expansão futura para automação, sensores, voz,
monitoramento e integração com o NoiseBot.

Regra principal do produto:

```text
O painel precisa continuar útil, responsivo e bonito
mesmo quando internet, API, memória, storage ou algum serviço falhar.
```

## 2. Escopo

### 2.1 Mantido no projeto

- Home simples/adaptativa progressiva
- data e hora
- Wi-Fi
- clima básico
- BTC e dólar via CoinGecko REST (snapshot)
- cache local
- tela de sistema/status
- configurações básicas
- logs básicos
- arquitetura offline-first
- preparação para LVGL
- preparação para server opcional futuro

### 2.2 Removido do projeto

- Controle de TV Samsung (e `SamsungTvAdapter`, Wake-on-LAN, SmartThings, IR
  para TV, tokens Samsung). Motivo: alto risco técnico, comportamento varia por
  modelo/ano, WebSocket/token/WoL incertos, valor insuficiente para o MVP.

### 2.3 Movido para futuro

Sonoff LAN (Tasmota/ESPHome/MQTT), cenas rápidas, planta da casa, automações
físicas, sensores (presença/luz/temperatura/ar), comandos de voz,
monitoramento/câmera/interfone, integração NoiseBot.

## 3. Hardware e riscos críticos

O ESP32-P4 **não possui Wi-Fi/Bluetooth nativo**. Nas placas Waveshare
`ESP32-P4-WIFI6`, a conectividade vem de um coprocessador ESP32-C6 via
ESP-Hosted/SDIO. Toda a rede (HTTPS, WebSocket, NTP, dispositivos LAN futuros)
passa pelo link P4 ↔ C6. Detalhes e checklist em `HARDWARE.md`.

## 4. MVP real (enxuto, ponta a ponta)

```text
boot estável → display/touch → Wi-Fi validado P4↔C6 → NTP/data/hora →
Home simples → clima básico → BTC snapshot → dólar snapshot → cache básico →
configurações Wi-Fi/brilho → tela sistema/status mínima → logs básicos
```

Fora do MVP: Samsung TV, Sonoff, Pomodoro, perfis, rotinas, dashboard adaptativo
completo, widgets rearranjáveis, álbum inteligente, câmera, voz, NoiseBot, mini
carteira, candles operacionais.

## 5. v1.0 - Painel Pessoal (após MVP validado)

Home adaptativa simples, modo noite, relógio premium, alertas simples de preço,
linha do tempo do dia, álbum inteligente, timer/Pomodoro, feedback sonoro,
perfis simples, rotinas locais básicas.

## 6. Estratégia de dados de mercado (ADR-0006)

Decisão: **CoinGecko REST** no MVP (já validado pelo usuário em outro projeto).

Política do `MarketService` no MVP:

```text
- atualização a cada 60s
- budget interno de até 6 chamadas/min
- cache obrigatório
- buscar em lote quando possível (1 chamada p/ BTC + favoritas)
- modo Home: resumo | modo Mercado: snapshot detalhado
- WebSocket de exchange: somente futuro
```

> Operacional: registrar a **Demo key gratuita** do CoinGecko (100 req/min) evita
> 429 do plano público (5-15/min). Fonte do par USD/BRL a confirmar (derivar do
> CoinGecko ou usar fonte de câmbio dedicada). Clima via provider próprio
> (sugestão: Open-Meteo, sem chave) - a definir em ADR específico.

## 7. Renderização e memória

Tela 1024×600 em RGB565: ~1.2 MB por framebuffer (~2.4 MB com double buffer) -
obrigatoriamente em PSRAM. Regras: render parcial, dirty rectangles, sem redraw
completo, candles incrementais, imagens pré-redimensionadas, JPEG decode
limitado, álbum pausado em telas pesadas. Proibições: redesenhar gráfico inteiro
a cada tick; decodificar foto grande durante tela financeira; **manipular
objetos LVGL fora da `lvgl_task`**.

## 8. Arquitetura (resumo)

```text
Provider → Service → StateStore → EventBus → UiDispatcher → lvgl_task → UI
```

Detalhes e regras em `ARCHITECTURE.md`.

## 9. Estrutura do monorepo

```text
NovaPainel/
├─ firmware/   # núcleo do produto (ESP-IDF/C++)
├─ server/     # opcional, futuro
├─ shared/     # contratos firmware <-> server (schemas/protocol)
├─ docs/       # esta documentação
└─ tools/      # scripts utilitários
```

## 10. Ordem de implementação

Ver `ROADMAP.md`. Resumo: primeiro validar hardware e rede (Fase 0), depois core
firmware, depois Home mínima, depois dados reais conservadores, depois v1.0.

## 11. Pendências imediatas

- confirmar modelo exato da placa comprada (P4+C6 ou P4+C5)
- confirmar BSP, flash/PSRAM e partições recomendadas
- confirmar se o C6 vem pré-flashado e o processo de update do C6
- confirmar display/touch/backlight
- confirmar presença (ou ausência) de RTC
- confirmar SD + Wi-Fi simultâneo

## 12. Conclusão

A abordagem prioriza validação técnica real antes de features: validar hardware
e rede → core firmware → Home mínima → dados reais conservadores → v1.0 e
roadmap. Isso reduz risco e evita retrabalho.
