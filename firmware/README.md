# NovaPainel - Firmware

Núcleo do produto. ESP-IDF + C++ (offline-first). Esta é a **estrutura inicial
(esqueleto)**: serviços, contratos e fluxo de dados completos, porém com
**mocks** no lugar de LVGL, BSP, rede e APIs reais.

## O que existe hoje

- `EventBus`, `StateStore`, `Service` / `ServiceManager`, `RequestOrchestrator`,
  `UiDispatcher` (core)
- `AppState` e modelos de dados (models)
- `MockBoard` (HAL) — simula boot/display/touch/rede
- `ClockService`, `MarketService`, `NotificationService`
- `MockMarketProvider` (snapshot fixo: BTC 104200, USD/BRL 5.42, +1.8%)
- `HomeScreen` que "renderiza" via logs (sem LVGL)

O `app_main` conecta tudo e roda um loop mock que imprime a Home periodicamente.

## O que NÃO existe ainda (intencional)

LVGL real, driver real de display/touch, Wi-Fi/HTTPS reais, CoinGecko real,
Sonoff, TV Samsung, cache em SD, server. Ver `docs/ROADMAP.md`.

## Build (ESP-IDF)

Requer ESP-IDF instalado (com suporte a ESP32-P4).

```bash
cd firmware
idf.py set-target esp32p4
idf.py build
idf.py -p <porta> flash monitor
```

> Importante: a placa-alvo usa ESP32-P4 **sem Wi-Fi nativo**; a conectividade
> depende do coprocessador ESP32-C6 via ESP-Hosted/SDIO, validado na Fase 0.
> Ver `docs/HARDWARE.md`.

## Estrutura

```text
firmware/
├─ CMakeLists.txt
├─ sdkconfig.defaults
├─ partitions.csv        # preliminar
├─ main/                 # app_main.cpp (wiring + loop mock)
├─ components/
│  ├─ core/              # EventBus, StateStore, ServiceManager, Orchestrator, UiDispatcher
│  ├─ models/            # AppState e structs de dados
│  ├─ board/             # HAL + MockBoard
│  ├─ services/          # ClockService, MarketService, NotificationService
│  ├─ providers/         # IMarketProvider + MockMarketProvider
│  ├─ ui/                # HomeScreen (logs)
│  └─ utils/             # Result<T> / Status
└─ tests/native/         # testes de host (futuro)
```
