# NovaPainel - Gate 15 coexistence harness (standalone ESP-IDF project).
# Purpose: prove that Wi-Fi (via ESP32-C6 / ESP-Hosted) + display framebuffer in
# PSRAM + touch can run simultaneously for 72h without resets. See
# docs/FASE0-CHECKLIST.md (Gate 15). This is a Fase 0 experiment, NOT product code,
# so it is intentionally separate from firmware/main and the product components.
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(gate15_coexistence)
