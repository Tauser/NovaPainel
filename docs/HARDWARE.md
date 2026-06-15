# NovaPainel - Hardware

## Premissa crítica: ESP32-P4 não tem Wi-Fi nativo

- O **ESP32-P4 não possui Wi-Fi/Bluetooth nativo**.
- Em placas Waveshare `ESP32-P4-WIFI6`, a conectividade vem de um coprocessador
  **ESP32-C6** via **ESP-Hosted/SDIO**.
- Consequência: **toda** comunicação de rede passa pelo link **P4 ↔ C6** —
  HTTPS, WebSocket, NTP, APIs externas e quaisquer dispositivos/serviços LAN
  futuros.

> Confirmar se a placa comprada usa **C6** (Wi-Fi 6, 2.4 GHz) ou **C5**
> (dual-band). O firmware-slave e o suporte do ESP-Hosted diferem.

## Fase 0 - Risk Gates (validar ANTES de features)

A Fase 0 trata a rede como **risco de hardware**, não como detalhe posterior.

```text
- firmware do C6 / ESP-Hosted (vem pré-flashado? como atualizar?)
- link P4 ↔ C6 via SDIO
- Wi-Fi real
- HTTPS básico
- NTP
- WebSocket simples (prova de futuro; não bloqueante p/ MVP)
- SD card
- SD + Wi-Fi simultâneo
- PSRAM detectada
- framebuffer em PSRAM
- display
- touch
- backlight
- RTC (existe? qual chip? mantém hora sem energia?)
- particionamento correto (inclui partição do slave fw do C6 e OTA futuro)
```

### Critério de saída da Fase 0

Boot e logs estáveis; Wi-Fi, HTTPS, NTP, display e touch funcionais; PSRAM
detectada; SD funcional (ou decisão consciente de adiar SD); particionamento
inicial validado.

## Relógio offline (ADR-0009)

A promessa de relógio offline depende de haver um RTC com bateria. Sem RTC:
mantém a hora enquanto ligado; após reboot sem internet, exibe horário como
**não sincronizado** e ressincroniza via NTP quando a rede voltar.

## Particionamento

`firmware/partitions.csv` é **preliminar** e não deve ser tratado como definitivo
até validar a placa real (tamanho de flash, partição do slave firmware do C6,
layout de OTA, NVS, cache/storage).

## Sensores futuros (prováveis)

```text
- LD2410C (presença mmWave) via UART
- BH1750  (luz ambiente)     via I2C
- BME280  (temp/umid/press)  via I2C
```

Ligações previstas:

```text
I2C header : BH1750, BME280
UART header: LD2410C
```

> Sensores são roadmap futuro (não entram no MVP), mas a arquitetura já reserva
> espaço para um `PresenceService`/sensores via mock até o hardware chegar.
