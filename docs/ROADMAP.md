# NovaPainel - Roadmap

> Estado atual: **Fase 2 concluída** (firmware core com mocks). LVGL, BSP real,
> rede e APIs reais ainda não implementados, por decisão.

```text
Fase 0  - Risk Gates de hardware (C6/ESP-Hosted, SDIO, Wi-Fi, HTTPS, NTP,
          WebSocket, SD+Wi-Fi, PSRAM, display/touch, RTC, partições)
Fase 1  - Organização do monorepo                                  [feito]
Fase 2  - Firmware core com mocks                                  [feito]
Fase 3  - Board real / BSP
Fase 4  - Display/touch/LVGL real
Fase 5  - Wi-Fi/NTP/HTTPS/CoinGecko snapshot
Fase 6  - Home MVP com cache
Fase 7  - Sistema/status/logs
Fase 8  - v1.0: modo noite, álbum, timer, perfis simples
Fase 9  - sensores: LD2410C, BH1750, BME280
Fase 10 - futuro: Sonoff/automação física
Fase 11 - futuro: server opcional/NoiseBot
```

## Detalhe das fases iniciais

- **Fase 2 (atual):** core (`EventBus`, `StateStore`, `Service`/`ServiceManager`,
  `RequestOrchestrator`, `UiDispatcher`), serviços mock (`ClockService`,
  `MarketService`, `NotificationService`), `MockBoard`, `MockMarketProvider`,
  `HomeScreen` por logs. Objetivo: validar o fluxo de dados ponta a ponta.
- **Fase 3:** substituir `MockBoard` pelo BSP Waveshare; só então confiar nos
  flags de `SystemStatus`.
- **Fase 4:** integrar LVGL real; `UiDispatcher::process_pending()` passa a rodar
  na `lvgl_task`; `HomeScreen::render` passa de logs para widgets.
- **Fase 5:** trocar `MockMarketProvider` por `CoinGeckoProvider` (REST, 60s,
  6 req/min, cache, batch). Adicionar provider de clima (ex.: Open-Meteo) e fonte
  USD/BRL.
