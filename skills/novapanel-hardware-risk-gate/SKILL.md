---
name: novapanel-hardware-risk-gate
description: Validate NovaPanel hardware risk gates for ESP32-P4-WIFI6-Touch-LCD-7B, ESP32-C6/ESP-Hosted, USB-JTAG, flash, PSRAM, display, touch, backlight, SD, NTP, HTTPS, partitions, and board/BSP bring-up. Use for Fase 0, Fase 3, or hardware debugging.
---

# NovaPanel Hardware Risk Gate

Use this skill for hardware-facing work.

## Read first

- `docs/HARDWARE.md`
- `docs/ROADMAP.md`
- `firmware/README.md`
- `firmware/partitions.csv`
- `firmware/sdkconfig.defaults`

## Fixed assumptions

- Build target is `esp32p4`.
- ESP-IDF path is expected to be `C:\esp\v5.5.4\esp-idf`.
- Local ESP-IDF version baseline is `v5.5.4`.
- The ESP32-P4 has no native Wi-Fi/Bluetooth.
- Network depends on the ESP32-C6 coprocessor through ESP-Hosted/SDIO.

## Risk gate order

Validate in this order:

1. Toolchain and target: `idf.py --version`, `idf.py set-target esp32p4`, `idf.py build`.
2. Flash/boot/monitor through builtin USB-JTAG unless the user is using ESP-PROG.
3. Flash size and partition table.
4. PSRAM detection.
5. Display power, backlight, and panel init.
6. Touch controller.
7. SD card.
8. ESP32-C6 firmware/update strategy.
9. P4-C6 SDIO/ESP-Hosted link.
10. Wi-Fi association.
11. NTP.
12. HTTPS.
13. SD + Wi-Fi simultaneous behavior.
14. RTC presence/absence and offline clock behavior.

## Rules

- Treat `MockBoard` readiness flags as simulation only.
- Do not trust `network_ready` until ESP-Hosted is proven on hardware.
- Do not finalize partition layout until flash size, OTA needs, storage, and C6 slave firmware strategy are confirmed.
- Record each validated item with date, command/log evidence, result, and follow-up.
- Add or update an ADR for non-trivial hardware strategy changes.
