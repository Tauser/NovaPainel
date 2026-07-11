# NovaPanel — Planejamento do Produto (baseline v4)

> Documento canônico de produto. Define o que o NovaPanel é, para quem, com
> que qualidade e o que fica de fora. Estado atual: ver `STATUS.md`.

## 1. Visão do produto

O **NovaPanel** é um smart display de parede/mesa de 7", local e
offline-first, que funciona como central pessoal da casa: hora e data
confiáveis, clima, mercado (BTC e USD/BRL), agenda, notificações e — em fases
futuras — automação e sensores locais.

Regra principal do produto:

```text
O painel precisa continuar útil, responsivo, bonito e previsível mesmo
quando internet, API, storage, memória ou algum serviço falhar.
```

Não é um terminal de nuvem. Internet melhora o produto; nunca define a
utilidade básica. Um `server/` opcional pode existir no futuro — o firmware
jamais depende estruturalmente dele.

## 2. Barra de qualidade "premium" (mensurável, não adjetivo)

O v4 herda do v3 a lição de que "funciona" não é critério. O produto só é
considerado premium quando TODAS as linhas abaixo têm evidência registrada:

| Dimensão | Critério objetivo |
|---|---|
| Fluidez | Interação touch sem stutter perceptível; repintura de tela < 100 ms; zero glitch visual (flash branco/tearing) em operação normal com rede ativa |
| Disponibilidade | Soak de 7 dias contínuos sem reboot não intencional, sem leak (watermark de heap estável) |
| Degradação | Queda de Wi-Fi/API/cache exercitada em teste, com UI sempre operável e dado stale sinalizado |
| Boot | < 10 s até tela útil; falha de periférico não gera boot loop cego (backoff + breadcrumb) |
| Atualização | OTA A/B com rollback automático — obrigatório ANTES de qualquer unidade lacrada com Flash Encryption RELEASE |
| Diagnóstico | Todo crash em campo produz coredump + reset reason recuperáveis |
| Segurança | Secure Boot v2 + Flash Encryption + NVS Encryption validados em bancada, com procedimento escrito |

## 3. Objetivos de engenharia

- escalável em telas e domínios de dados sem colapsar a manutenção;
- robusto contra falha de rede, reboot, dado parcial e payload hostil;
- testável no host (lógica) e com fixtures (parsing) — não só na placa;
- documentação confiável por construção (estado em um único arquivo);
- respeito explícito ao orçamento físico da placa (`RESOURCE-BUDGET.md`).

Princípios obrigatórios (contratos, detalhados no `ARCHITECTURE.md`):
`offline-first` · `state-driven UI` · `ports and adapters de verdade`
(interfaces implementadas, não prometidas) · `single source of truth`
(StateStore) · `safe concurrency` (ownership declarado) · `graceful
degradation` · `small recoverable steps`.

## 4. Escopo funcional

### 4.1 MVP (Fases 1–7 do ROADMAP)

- Home adaptativa: hora/data, clima, BTC, USD/BRL, notificações;
- onboarding Wi-Fi com wizard e reconfiguração posterior;
- clima com previsão horária/diária (Open-Meteo), localização configurável;
- mercado: BTC spot + OHLC/candles (CoinGecko), USD/BRL (AwesomeAPI);
- cache local com stale explícito em todos os domínios;
- settings: brilho, tema, formato de hora, volume, rede, sistema;
- tela de sistema/diagnóstico (reset reason, reboots, heap, rede);
- observabilidade de campo (coredump, contadores persistidos);
- OTA A/B + release PROD seguro.

### 4.2 Pós-MVP planejado (v1.0+)

Modo noite, timer/Pomodoro, álbum/fotoframe, agenda funcional, perfis;
depois: Sonoff LAN/Tasmota/ESPHome/MQTT, cenas, sensores locais, voz,
NoiseBot, server opcional.

### 4.3 Fora de escopo permanente (salvo ADR revogando)

- Controle de TV Samsung (WoL, SmartThings, IR, tokens) — risco alto,
  valor baixo (decisão herdada do v3);
- mercado em tempo real via WebSocket no MVP (REST com intervalo é
  suficiente e cabe no orçamento de rede);
- dependência obrigatória de backend.

### 4.4 Suposições de produto assumidas conscientemente

- Produto pessoal, 1 unidade, 1 idioma (pt-BR), fuso brasileiro. Strings
  centralizadas para permitir i18n futura barata, mas i18n NÃO é requisito
  do MVP (ADR-0011 v4).
- APIs gratuitas (CoinGecko free tier, Open-Meteo, AwesomeAPI) com rate
  limits respeitados por política central.

## 5. Hardware alvo

Waveshare **ESP32-P4-WIFI6-Touch-LCD-7B**: P4 (dual-core RISC-V, PSRAM 32 MB,
sem rádio) + C6 (Wi-Fi 6 via ESP-Hosted/SDIO), display 7" 1024×600 MIPI-DSI
(EK79007), touch GT911, codec ES8311, RTC com bateria, SD. Fatos e gotchas:
`HARDWARE.md`. Limites físicos e regras de alocação: `RESOURCE-BUDGET.md`
— ambos herdam validação de bancada do baseline v3 (não refazer).

## 6. Estratégia técnica base

- Firmware ESP-IDF + LVGL, C++17, monorepo.
- Arquitetura em camadas com fluxo único de dados
  (`Provider → Service → StateStore → EventBus → UI`), UI por registro de
  telas (`UI-PATTERN.md`), hardware atrás de `board/`.
- Rede serializada (1 HTTPS por vez) coordenada por política central —
  decisão validada em campo no v3 (ADR-0004 v4).
- Persistência: NVS (config, versionada) + LittleFS (cache, atômico).
- Testes: host-first para lógica; fixtures para parsing; placa para BSP;
  soak para operação (`TESTING.md`).

## 7. Riscos de produto aceitos e monitorados

1. **Física da placa**: PSRAM/flash/DSI compartilham barramento — features de
   mídia (álbum, áudio contínuo) podem custar mais do que o planejado. Toda
   feature nova declara seu custo contra o `RESOURCE-BUDGET.md` antes de
   entrar no roadmap.
2. **APIs gratuitas** podem mudar contrato ou limitar — mitigado por
   interface de provider + fixture + circuit breaker.
3. **Bus factor 1** — mitigado por ADRs, STATUS único e proibição de
   conhecimento apenas-em-comentário para invariantes de plataforma.
