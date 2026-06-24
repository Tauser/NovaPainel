#!/usr/bin/env bash
# NovaPainel - host_check.sh
#
# Validates the firmware C++ on the HOST (no ESP-IDF / no hardware needed).
# It generates tiny shim headers for esp_log.h / esp_timer.h / freertos and
# compiles every firmware component with g++ (-Wall -Wextra). This catches
# typos, missing includes, template/type errors and broken interfaces quickly.
#
# It does NOT replace `idf.py build`; it is a fast pre-flight for the
# platform-independent logic in core/, models/, services/, providers/, utils/.
#
# Some .cpp files are inherently hardware-only (real BSP/Wi-Fi/display
# drivers with no host shim, e.g. WaveshareBoard) and are skipped - see
# SKIP_FILES below. Their headers (e.g. waveshare_board.hpp) stay
# dependency-free on purpose so files that include them (app_main.cpp) are
# still host-checkable.
#
# Usage:
#   bash tools/scripts/host_check.sh
#   bash tools/scripts/host_check.sh --app   # also syntax-check main/app_main.cpp
#
# Exit code: 0 if everything compiles, 1 otherwise.
set -euo pipefail

# Resolve repo root relative to this script (works from anywhere).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FW="$ROOT/firmware"
COMP="$FW/components"

CXX="${CXX:-g++}"
STD="${CXXSTD:-gnu++17}"
CHECK_APP=0
[ "${1:-}" = "--app" ] && CHECK_APP=1

if ! command -v "$CXX" >/dev/null 2>&1; then
  echo "ERROR: $CXX not found. Install g++ (or set CXX) to run host_check." >&2
  exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
SHIM="$WORK/shim"; OBJ="$WORK/obj"
mkdir -p "$SHIM/freertos" "$OBJ"

# ---- shim headers (stand in for ESP-IDF on the host) ----
cat > "$SHIM/esp_log.h" <<'SH'
#pragma once
#include <cstdio>
#define ESP_LOGI(tag, fmt, ...) do{ printf("[I] %s: " fmt "\n", tag, ##__VA_ARGS__); }while(0)
#define ESP_LOGW(tag, fmt, ...) do{ printf("[W] %s: " fmt "\n", tag, ##__VA_ARGS__); }while(0)
#define ESP_LOGE(tag, fmt, ...) do{ printf("[E] %s: " fmt "\n", tag, ##__VA_ARGS__); }while(0)
#define ESP_LOGD(tag, fmt, ...) do{ }while(0)
SH
cat > "$SHIM/esp_timer.h" <<'SH'
#pragma once
#include <cstdint>
extern "C" int64_t esp_timer_get_time(void);
SH
cat > "$SHIM/freertos/FreeRTOS.h" <<'SH'
#pragma once
#include <cstdint>
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
SH
cat > "$SHIM/freertos/task.h" <<'SH'
#pragma once
#include "freertos/FreeRTOS.h"
extern "C" void vTaskDelay(TickType_t);
SH

# ---- include path: every component's include/ + the shim ----
INC=("-I$SHIM")
for d in "$COMP"/*/include; do INC+=("-I$d"); done

# ---- files that need real ESP-IDF/BSP headers with no host shim ----
SKIP_FILES=(
  "waveshare_board.cpp"  # real display/touch BSP + Wi-Fi - hardware only
  "home_screen.cpp"      # real LVGL widgets (Fase 4) - lvgl.h has no host shim
)
is_skipped() {
  local base="$(basename "$1")"
  for skip in "${SKIP_FILES[@]}"; do
    [ "$base" = "$skip" ] && return 0
  done
  return 1
}

echo "NovaPainel host_check"
echo "  compiler : $($CXX --version | head -1)"
echo "  std      : $STD"
echo "  root     : $ROOT"
echo

fail=0
shopt -s nullglob
for src in \
  "$COMP"/utils/src/*.cpp \
  "$COMP"/models/src/*.cpp \
  "$COMP"/core/src/*.cpp \
  "$COMP"/board/src/*.cpp \
  "$COMP"/providers/src/*.cpp \
  "$COMP"/services/src/*.cpp \
  "$COMP"/ui/src/**/*.cpp \
  "$COMP"/ui/src/*.cpp ; do
  [ -f "$src" ] || continue
  rel="${src#$ROOT/}"
  if is_skipped "$src"; then
    printf "  SKIP  %s (hardware-only, no host shim)\n" "$rel"
    continue
  fi
  if "$CXX" -std="$STD" -Wall -Wextra -c "${INC[@]}" "$src" -o "$OBJ/$(echo "$rel" | tr '/' '_').o" 2>"$WORK/err.txt"; then
    printf "  OK    %s\n" "$rel"
  else
    printf "  FAIL  %s\n" "$rel"; sed 's/^/        /' "$WORK/err.txt"; fail=1
  fi
done

if [ "$CHECK_APP" = "1" ]; then
  src="$FW/main/app_main.cpp"
  if "$CXX" -std="$STD" -Wall -Wextra -c "${INC[@]}" "$src" -o "$OBJ/app_main.o" 2>"$WORK/err.txt"; then
    printf "  OK    %s\n" "firmware/main/app_main.cpp"
  else
    printf "  FAIL  %s\n" "firmware/main/app_main.cpp"; sed 's/^/        /' "$WORK/err.txt"; fail=1
  fi
fi

echo
if [ "$fail" -eq 0 ]; then
  echo "host_check: PASS"
else
  echo "host_check: FAIL"
fi
exit "$fail"
