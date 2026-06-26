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
- data e hora (ClockService híbrido RTC↔NTP — ADR-0009/0032)
- Wi-Fi (wizard de onboarding + auto-reconexão — ADR-0017)
- clima básico (Open-Meteo, sem chave — ADR-0022/0023)
- BTC e dólar via CoinGecko REST + ForexProvider (snapshot 60s — ADR-0006/0021/0026)
- cache local (LittleFS, escrita atômica, stale visível — ADR-0015/0027)
- tela de sistema/status (SystemScreen + SystemService — ADR-0028)
- configurações básicas (wizard + SetupService NVS — ADR-0017)
- logs básicos + coredump em flash (ADR-0014/0028)
- arquitetura offline-first (circuit breaker + backoff — ADR-0012/0029)
- LVGL real (BSP Waveshare, dirty rect nativo, buffer PSRAM — ADR-0018/0031)
- server opcional futuro (arquitetura permite; firmware nunca depende — ADR-0002)

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

**Entregue (Fases 1-11):**

```text
boot estável → display/touch (EK79007 + GT911) → Wi-Fi validado P4↔C6 (Gates 1-16) →
NTP/data/hora (ClockService + RTC interno) → Home simples (relógio, clima, BTC, USD/BRL) →
clima real (Open-Meteo) → BTC/USD snapshot (CoinGecko) → USD/BRL snapshot (ForexProvider) →
cache LittleFS (dado stale visível, sobrevive reboot) → wizard onboarding (nome, Wi-Fi,
fuso, formato) → tela sistema/status (motivo reset, reboots, idade dos dados) →
circuit breaker + backoff por domínio → build PROD seguro (Secure Boot v2 + Flash Encryption)
→ buffer display PSRAM (quarter-height single-buffer, sw_rotate) → NotificationService fila prioritária
```

Fora do MVP: Samsung TV (removido), Sonoff, Pomodoro, perfis, rotinas, dashboard adaptativo
completo, widgets rearranjáveis, álbum inteligente, câmera, voz, NoiseBot, mini
carteira, candles operacionais.

## 5. v1.0 - Painel Pessoal (após MVP validado)

Home adaptativa simples, modo noite, relógio premium, alertas simples de preço,
linha do tempo do dia, álbum inteligente, timer/Pomodoro, feedback sonoro,
perfis simples, rotinas locais básicas.

## 6. Estratégia de dados de mercado (ADR-0006/0021/0022/0023/0026)

Decisão: **CoinGecko REST** no MVP (validado na placa física).

Política do `MarketService` no MVP:

```text
- atualização a cada 60s (MarketService) / 120s (ForexService) / 10min (WeatherService)
- budget interno de até 6 chamadas/min por domínio (RequestOrchestrator)
- circuit breaker por domínio: 3 falhas → Open, backoff 10s→5min (ADR-0029)
- cache LittleFS obrigatório (CacheStore, ADR-0027): dado stale mostrado como "(cache)"
- busca em lote: 1 chamada p/ BTC+favoritas no CoinGecko
- WebSocket de exchange: somente futuro
```

> **Operacional:** registrar a Demo key gratuita do CoinGecko (100 req/min) evita 429.
> **USD/BRL:** `ForexProvider` via AwesomeAPI (câmbio dedicado, independente do CoinGecko — ADR-0026).
> **Clima:** `OpenMeteoProvider` (Open-Meteo, sem chave — ADR-0022/0023), localização fixa Brasília no MVP.

## 7. Renderização e memória

Tela 1024×600 em RGB565: ~1.2 MB por framebuffer (~2.4 MB com double buffer).
**Implementado (Fase 10, ADR-0031):** buffer de draw em PSRAM, quarter-height
(1024×150px × 2B = ~300KB), single-buffer. LVGL dirty rectangles nativo:
só as regiões alteradas são enviadas ao display (LVGL rastreia automaticamente).
Nota: `double_buffer=false` é obrigatório com `sw_rotate+PSRAM` — overlap entre
flush e próxima renderização causava flash esporádico no display.

Regras ainda válidas: sem redraw completo, candles incrementais pós-MVP,
imagens pré-redimensionadas no host, JPEG decode limitado, álbum pausado em
telas pesadas. Proibição inviolável: **nunca manipular objetos LVGL fora da
`lvgl_task`** (ADR-0007/0013).

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

## 11. Estado atual e pendências

**Confirmado em hardware (Fase 0, Gates 1-16 PASS):**
- Placa: Waveshare ESP32-P4-WIFI6-Touch-LCD-7B (P4+C6, 32MB flash, 32MB PSRAM)
- Display EK79007 (1024×600) + touch GT911 via BSP oficial
- ESP-Hosted/SDIO P4↔C6 funcional (Wi-Fi + display simultâneos — Gate 15, soak 8h)
- SD card via SDMMC + LDO canal 4
- RTC: domínio interno ESP32-P4 com bateria 1220 (sem chip I2C externo), decisão
  de clock offline fechada na Fase 0
- Brownout/térmica sob carga validados no Gate 16
- Partições: `nvs` + `nvs_keys` + `phy_init` + `factory` + `storage` + `coredump`

**Pendente:**
- Localização no wizard (Open-Meteo usa Brasília fixo atualmente)
- Tela de Configurações (edição pós-onboarding de Wi-Fi/fuso/nome)
- Trilha H de melhorias técnicas pós-Fase 0 em `ROADMAP.md`: H0-H9 fechados.
  Referências operacionais: `docs/SECURITY-OPERATIONS.md`,
  `docs/FIELD-OPERATIONS.md`, `docs/RELEASE-ROLLBACK.md` e
  `docs/SOAK-VALIDATION.md`.

**Próximas fases (v1.0+):**
- Fase 12: modo noite, álbum, timer/Pomodoro, perfis simples (ver ROADMAP)

## 12. Conclusão

A abordagem priorizou validação técnica real antes de features: hardware/rede
(Fase 0) → core firmware (Fases 1-2) → BSP/display real (Fases 3-4) → dados
reais/onboarding (Fase 5) → cache/resiliência/segurança/performance (Fases 6-11).
MVP entregue e validado na placa física. Próximo passo: v1.0 com expansão de
features (Fase 12+, ver ROADMAP).
