# NovaPanel Firmware

This is the rewritten firmware baseline for the ESP32-P4-WIFI6-Touch-LCD-7B.

The previous firmware was preserved at `../firmware_legacy_reference` for
consultation and selective reuse. This new tree starts with a small
hardware-first checkpoint:

- ESP-IDF project scaffold.
- Waveshare BSP dependency.
- LVGL display start.
- Dark boot screen loaded before enabling the backlight.
- Periodic uptime and heap telemetry.

Network, providers, storage services, UI modules, and setup flows should be
reintroduced incrementally after this display baseline is stable on hardware.
