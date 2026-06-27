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
#   bash tools/scripts/host_check.sh --tests # build and run native core tests
#
# Exit code: 0 if everything compiles, 1 otherwise.
set -euo pipefail

# Resolve repo root relative to this script (works from anywhere).
SCRIPT_PATH="${BASH_SOURCE[0]}"
SCRIPT_DIR_PART="${SCRIPT_PATH%/*}"
if [ "$SCRIPT_DIR_PART" = "$SCRIPT_PATH" ]; then
  SCRIPT_DIR_PART="."
fi
SCRIPT_DIR="$(cd "$SCRIPT_DIR_PART" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FW="$ROOT/firmware"
COMP="$FW/components"

CXX="${CXX:-g++}"
STD="${CXXSTD:-gnu++17}"
CHECK_APP=0
CHECK_TESTS=0

to_win_path() {
  pwsh.exe -NoLogo -NoProfile -NonInteractive -Command \
    "& { param([string]\$Path) [System.IO.Path]::GetFullPath(\$Path) }" "$1"
}

make_temp_dir() {
  pwsh.exe -NoLogo -NoProfile -NonInteractive -Command \
    '$dir = Join-Path ([System.IO.Path]::GetTempPath()) ([System.Guid]::NewGuid().ToString()); New-Item -ItemType Directory -Path $dir | Out-Null; Write-Output $dir'
}

cleanup_temp_dir() {
  pwsh.exe -NoLogo -NoProfile -NonInteractive -Command \
    "& { param([string]\$Path) if (Test-Path -LiteralPath \$Path) { Remove-Item -LiteralPath \$Path -Recurse -Force } }" "$1" >/dev/null 2>&1 || true
}

ensure_dir() {
  pwsh.exe -NoLogo -NoProfile -NonInteractive -Command \
    "& { param([string]\$Path) New-Item -ItemType Directory -Force -Path \$Path | Out-Null }" "$1"
}

write_stdin_file() {
  local path="$1"
  local content="" line=""
  while IFS= read -r line || [ -n "$line" ]; do
    content+="$line"$'\n'
  done
  printf '%s' "$content" > "$path"
}

print_prefixed_file() {
  local file="$1"
  [ -f "$file" ] || return 0
  while IFS= read -r line; do
    printf "        %s\n" "$line"
  done < "$file"
}

while [ $# -gt 0 ]; do
  case "$1" in
    --app) CHECK_APP=1 ;;
    --tests) CHECK_TESTS=1 ;;
    *)
      echo "ERROR: unknown option '$1'" >&2
      echo "Usage: bash tools/scripts/host_check.sh [--app] [--tests]" >&2
      exit 1
      ;;
  esac
  shift
done

if ! command -v "$CXX" >/dev/null 2>&1; then
  echo "ERROR: $CXX not found. Install g++ (or set CXX) to run host_check." >&2
  exit 1
fi
CXX_BIN="$(command -v "$CXX")"
CXX_WIN="$(to_win_path "$CXX_BIN")"

WORK="$(make_temp_dir)"
trap 'cleanup_temp_dir "$WORK"' EXIT
SHIM="$WORK/shim"; OBJ="$WORK/obj"
ensure_dir "$SHIM/freertos"
ensure_dir "$OBJ"
ensure_dir "$SHIM/bsp"
SHIM_WIN="$(to_win_path "$SHIM")"
OBJ_WIN="$(to_win_path "$OBJ")"
COMPILE_PS1="$WORK/compile.ps1"
LINK_PS1="$WORK/link.ps1"
RUN_PS1="$WORK/run.ps1"

write_stdin_file "$COMPILE_PS1" <<'PS'
param(
  [string]$Compiler,
  [string]$Std,
  [string]$Out
)

$args = @("-c", "-std=$Std", "-Wall", "-Wextra")
if ($env:INCLUDES_CSV) {
  $args += $env:INCLUDES_CSV -split ';'
}
if ($env:SOURCES_CSV) {
  $args += $env:SOURCES_CSV -split ';'
}
$args += @("-o", $Out)

& $Compiler @args
exit $LASTEXITCODE
PS

write_stdin_file "$LINK_PS1" <<'PS'
param(
  [string]$Compiler,
  [string]$Std,
  [string]$Out
)

$args = @("-std=$Std", "-Wall", "-Wextra")
if ($env:INCLUDES_CSV) {
  $args += $env:INCLUDES_CSV -split ';'
}
if ($env:SOURCES_CSV) {
  $args += $env:SOURCES_CSV -split ';'
}
$args += @("-o", $Out)

& $Compiler @args
exit $LASTEXITCODE
PS

write_stdin_file "$RUN_PS1" <<'PS'
param(
  [string]$Exe
)

& $Exe
exit $LASTEXITCODE
PS

# ---- shim headers (stand in for ESP-IDF on the host) ----
write_stdin_file "$SHIM/esp_log.h" <<'SH'
#pragma once
#include <cstdio>
#define ESP_LOGI(tag, fmt, ...) do{ printf("[I] %s: " fmt "\n", tag, ##__VA_ARGS__); }while(0)
#define ESP_LOGW(tag, fmt, ...) do{ printf("[W] %s: " fmt "\n", tag, ##__VA_ARGS__); }while(0)
#define ESP_LOGE(tag, fmt, ...) do{ printf("[E] %s: " fmt "\n", tag, ##__VA_ARGS__); }while(0)
#define ESP_LOGD(tag, fmt, ...) do{ }while(0)
SH
write_stdin_file "$SHIM/esp_timer.h" <<'SH'
#pragma once
#include <cstdint>
extern "C" int64_t esp_timer_get_time(void);
SH
write_stdin_file "$SHIM/esp_heap_caps.h" <<'SH'
#pragma once
#include <cstddef>
#define MALLOC_CAP_INTERNAL 0x1u
#define MALLOC_CAP_SPIRAM 0x2u
extern "C" std::size_t heap_caps_get_free_size(unsigned);
extern "C" std::size_t heap_caps_get_minimum_free_size(unsigned);
SH
write_stdin_file "$SHIM/freertos/FreeRTOS.h" <<'SH'
#pragma once
#include <cstdint>
typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
SH
write_stdin_file "$SHIM/freertos/task.h" <<'SH'
#pragma once
#include "freertos/FreeRTOS.h"
extern "C" void vTaskDelay(TickType_t);
SH
write_stdin_file "$SHIM/freertos/queue.h" <<'SH'
#pragma once
#include <cstdint>
typedef void* QueueHandle_t;
typedef int BaseType_t;
#define pdTRUE 1
#define pdFALSE 0
extern "C" QueueHandle_t xQueueCreate(uint32_t, uint32_t);
extern "C" BaseType_t xQueueSend(QueueHandle_t, const void*, TickType_t);
extern "C" BaseType_t xQueueReceive(QueueHandle_t, void*, TickType_t);
SH
write_stdin_file "$SHIM/esp_system.h" <<'SH'
#pragma once
inline void esp_restart(void) {}
SH
write_stdin_file "$SHIM/lvgl.h" <<'SH'
#pragma once

#include <cstddef>
#include <cstdint>

struct lv_obj_t;
struct lv_display_t;
struct lv_event_t;
struct lv_font_t;

using lv_coord_t = int32_t;

struct lv_color_t {
  uint32_t full{0};
};

static inline lv_color_t lv_color_hex(uint32_t value) {
  return lv_color_t{value};
}

constexpr int LV_OPA_TRANSP = 0;
constexpr int LV_OPA_COVER = 255;
constexpr int LV_OPA_30 = 77;
constexpr int LV_FLEX_FLOW_COLUMN = 1;
constexpr int LV_FLEX_FLOW_ROW = 2;
constexpr int LV_FLEX_ALIGN_START = 0;
constexpr int LV_FLEX_ALIGN_CENTER = 1;
constexpr int LV_FLEX_ALIGN_END = 2;
constexpr int LV_FLEX_ALIGN_SPACE_BETWEEN = 3;
constexpr int LV_SCROLLBAR_MODE_OFF = 0;
constexpr int LV_SIZE_CONTENT = -1;
constexpr int LV_ANIM_OFF = 0;
constexpr int LV_TEXT_ALIGN_CENTER = 0;
constexpr int LV_ALIGN_CENTER = 0;
constexpr int LV_BORDER_SIDE_BOTTOM = 1;
constexpr int LV_BORDER_SIDE_LEFT = 2;
constexpr int LV_STATE_CHECKED = 1;
constexpr int LV_STATE_DISABLED = 2;
constexpr int LV_PART_MAIN = 0;
constexpr int LV_PART_INDICATOR = 1;
constexpr int LV_PART_KNOB = 2;
constexpr int LV_OBJ_FLAG_HIDDEN = 1 << 0;
constexpr int LV_OBJ_FLAG_CHECKABLE = 1 << 1;
constexpr int LV_LABEL_LONG_DOT = 0;
constexpr int LV_DISPLAY_ROTATION_180 = 180;

#define LV_FONT_DECLARE(name) extern const lv_font_t name;

extern const lv_font_t lv_font_montserrat_14;
extern const lv_font_t lv_font_montserrat_16;
extern const lv_font_t lv_font_montserrat_20;
extern const lv_font_t lv_font_montserrat_24;
extern const lv_font_t lv_font_montserrat_28;
extern const lv_font_t lv_font_montserrat_32;
extern const lv_font_t lv_font_montserrat_48;

static inline lv_coord_t lv_pct(int32_t value) {
  return static_cast<lv_coord_t>(value);
}

extern "C" lv_obj_t* lv_obj_create(lv_obj_t* parent);
extern "C" lv_obj_t* lv_button_create(lv_obj_t* parent);
extern "C" lv_obj_t* lv_switch_create(lv_obj_t* parent);
extern "C" lv_obj_t* lv_label_create(lv_obj_t* parent);
extern "C" lv_obj_t* lv_bar_create(lv_obj_t* parent);
extern "C" lv_obj_t* lv_screen_active(void);
extern "C" void lv_label_set_text(lv_obj_t*, const char*);
extern "C" void lv_obj_set_style_text_font(lv_obj_t*, const lv_font_t*, int);
extern "C" void lv_obj_set_style_text_color(lv_obj_t*, lv_color_t, int);
extern "C" void lv_obj_set_style_text_align(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_bg_color(lv_obj_t*, lv_color_t, int);
extern "C" void lv_obj_set_style_bg_opa(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_border_color(lv_obj_t*, lv_color_t, int);
extern "C" void lv_obj_set_style_border_width(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_border_opa(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_border_side(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_pad_all(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_pad_hor(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_pad_ver(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_pad_row(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_pad_column(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_pad_left(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_pad_bottom(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_radius(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_shadow_width(lv_obj_t*, int, int);
extern "C" void lv_obj_set_style_flex_grow(lv_obj_t*, int, int);
extern "C" void lv_obj_set_scrollbar_mode(lv_obj_t*, int);
extern "C" void lv_obj_set_size(lv_obj_t*, lv_coord_t, lv_coord_t);
extern "C" void lv_obj_set_pos(lv_obj_t*, lv_coord_t, lv_coord_t);
extern "C" void lv_obj_set_height(lv_obj_t*, lv_coord_t);
extern "C" void lv_obj_set_width(lv_obj_t*, lv_coord_t);
extern "C" void lv_obj_align(lv_obj_t*, int, lv_coord_t, lv_coord_t);
extern "C" void lv_obj_add_flag(lv_obj_t*, int);
extern "C" void lv_obj_clear_flag(lv_obj_t*, int);
extern "C" void lv_obj_add_state(lv_obj_t*, int);
extern "C" void lv_obj_clear_state(lv_obj_t*, int);
extern "C" void lv_obj_set_flex_flow(lv_obj_t*, int);
extern "C" void lv_obj_set_flex_align(lv_obj_t*, int, int, int);
extern "C" void lv_obj_set_grid_dsc_array(lv_obj_t*, const int32_t*, const int32_t*);
extern "C" void lv_bar_set_range(lv_obj_t*, int, int);
extern "C" void lv_bar_set_value(lv_obj_t*, int, int);
extern "C" void lv_obj_set_scrollbar_mode(lv_obj_t*, int);
extern "C" void lv_label_set_long_mode(lv_obj_t*, int);
extern "C" void lv_obj_add_event_cb(lv_obj_t*, void (*)(lv_event_t*), int, void*);
SH
write_stdin_file "$SHIM/bsp/esp32_p4_wifi6_touch_lcd_7b.h" <<'SH'
#pragma once

#include <cstddef>
#include <cstdint>

#include "lvgl.h"

#define BSP_LCD_H_RES 1024
#define BSP_LCD_V_RES 600

struct esp_lvgl_port_cfg_t {
  int task_stack{0};
};

struct bsp_display_cfg_t {
  esp_lvgl_port_cfg_t lvgl_port_cfg{};
  std::size_t buffer_size{0};
  bool double_buffer{false};
  struct {
    bool buff_dma{false};
    bool buff_spiram{false};
    bool sw_rotate{false};
  } flags{};
};

#define ESP_LVGL_PORT_INIT_CONFIG() esp_lvgl_port_cfg_t{}

extern "C" lv_display_t* bsp_display_start_with_config(const bsp_display_cfg_t*);
extern "C" void bsp_display_rotate(lv_display_t*, int);
extern "C" bool bsp_display_lock(int);
extern "C" void bsp_display_unlock(void);
extern "C" void bsp_display_backlight_on(void);
SH

# ---- include path: every component's include/ + the shim ----
INC=("-I$SHIM_WIN")
for d in "$COMP"/*/include; do INC+=("-I$d"); done
for i in "${!INC[@]}"; do
  case "${INC[$i]}" in
    -I/*) INC[$i]="-I$(to_win_path "${INC[$i]:2}")" ;;
  esac
done

# ---- files that need real ESP-IDF/BSP headers with no host shim ----
SKIP_FILES=(
  "waveshare_board.cpp"   # real display/touch BSP + Wi-Fi - hardware only
  "novapanel_ui.cpp"      # real LVGL shell + screen wiring - host shim would be large
  "screen_home.cpp"       # real LVGL widgets (Fase 5) - host shim would be large
  "screen_boot.cpp"       # real LVGL widgets (Fase 5) - host shim would be large
  "screen_setup.cpp"      # real LVGL widgets (Fase 5) - host shim would be large
  "screen_agenda.cpp"     # real LVGL widgets (Fase 5) - host shim would be large
  "screen_market.cpp"     # real LVGL widgets (Fase 5) - host shim would be large
  "screen_settings.cpp"   # real LVGL widgets (Fase 5) - host shim would be large
  "screen_weather.cpp"    # real LVGL widgets (Fase 5) - host shim would be large
  "screen_placeholders.cpp" # real LVGL widgets (Fase 5) - host shim would be large
  "nova_keyboard_manager.cpp" # real LVGL keyboard manager - host shim would be large
  "setup_service.cpp"    # real NVS + esp_wifi (Fase 5, ADR-0017) - hardware only
  "clock_service.cpp"    # setenv/localtime_r (POSIX, in ESP-IDF's newlib) - MinGW/MSYS2 host libc lacks them
  "coingecko_provider.cpp" # real esp_http_client + mbedtls + cJSON (Fase 5, ADR-0021) - hardware only
  "open_meteo_provider.cpp" # real esp_http_client + mbedtls + cJSON (Fase 5, ADR-0022) - hardware only
  "forex_provider.cpp"   # real esp_http_client + mbedtls + cJSON (Fase 5 follow-up) - hardware only
  "cache_store.cpp"      # real esp_littlefs + ESP-IDF VFS (Fase 6, ADR-0027) - hardware only
  "system_service.cpp"   # real esp_reset_reason + NVS (Fase 7, ADR-0028) - hardware only
)
is_skipped() {
  local path="$1"
  local base="${path##*/}"
  for skip in "${SKIP_FILES[@]}"; do
    [ "$base" = "$skip" ] && return 0
  done
  return 1
}

compiler_version="$("$CXX" --version 2>/dev/null || true)"
compiler_version="${compiler_version%%$'\n'*}"

echo "NovaPainel host_check"
echo "  compiler : ${compiler_version:-$CXX_BIN}"
echo "  std      : $STD"
echo "  root     : $ROOT"
echo

fail=0
shopt -s nullglob globstar
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
  SRC_WIN="$(to_win_path "$src")"
  out_name="${rel//\//_}.o"
  OUT_WIN="$(to_win_path "$OBJ/$out_name")"
  includes_csv="$(IFS=';'; echo "${INC[*]}")"
  if INCLUDES_CSV="$includes_csv" SOURCES_CSV="$SRC_WIN" \
    pwsh.exe -NoLogo -NoProfile -NonInteractive -File "$COMPILE_PS1" \
      -Compiler "$CXX_WIN" \
      -Std "$STD" \
      -Out "$OUT_WIN" >"$WORK/out.txt" 2>"$WORK/err.txt"; then
    printf "  OK    %s\n" "$rel"
  else
    printf "  FAIL  %s\n" "$rel"
    print_prefixed_file "$WORK/err.txt"
    print_prefixed_file "$WORK/out.txt"
    fail=1
  fi
done

if [ "$CHECK_APP" = "1" ]; then
  src="$FW/main/app_main.cpp"
  SRC_WIN="$(to_win_path "$src")"
  OUT_WIN="$(to_win_path "$OBJ/app_main.o")"
  includes_csv="$(IFS=';'; echo "${INC[*]}")"
  if INCLUDES_CSV="$includes_csv" SOURCES_CSV="$SRC_WIN" \
    pwsh.exe -NoLogo -NoProfile -NonInteractive -File "$COMPILE_PS1" \
      -Compiler "$CXX_WIN" \
      -Std "$STD" \
      -Out "$OUT_WIN" >"$WORK/out.txt" 2>"$WORK/err.txt"; then
    printf "  OK    %s\n" "firmware/main/app_main.cpp"
  else
    printf "  FAIL  %s\n" "firmware/main/app_main.cpp"
    print_prefixed_file "$WORK/err.txt"
    print_prefixed_file "$WORK/out.txt"
    fail=1
  fi
fi

if [ "$CHECK_TESTS" = "1" ]; then
  test_src="$FW/tests/native/core_tests.cpp"
  test_src_win="$(to_win_path "$test_src")"
  test_bin_win="$(to_win_path "$OBJ/core_tests.exe")"
  includes_csv="$(IFS=';'; echo "${INC[*]}")"
  sources_csv="$(to_win_path "$COMP/models/src/app_state.cpp");$(to_win_path "$COMP/core/src/event_bus.cpp");$(to_win_path "$COMP/core/src/state_store.cpp");$(to_win_path "$COMP/core/src/request_orchestrator.cpp");$test_src_win"
  if INCLUDES_CSV="$includes_csv" SOURCES_CSV="$sources_csv" \
    pwsh.exe -NoLogo -NoProfile -NonInteractive -File "$LINK_PS1" \
      -Compiler "$CXX_WIN" \
      -Std "$STD" \
      -Out "$test_bin_win" >"$WORK/out.txt" 2>"$WORK/err.txt"; then
    printf "  OK    %s\n" "firmware/tests/native/core_tests.cpp"
    if pwsh.exe -NoLogo -NoProfile -NonInteractive -File "$RUN_PS1" -Exe "$test_bin_win" >"$WORK/test_out.txt" 2>"$WORK/test_err.txt"; then
      print_prefixed_file "$WORK/test_out.txt"
    else
      printf "  FAIL  %s\n" "firmware/tests/native/core_tests.cpp"
      print_prefixed_file "$WORK/test_err.txt"
      print_prefixed_file "$WORK/out.txt"
      fail=1
    fi
  else
    printf "  FAIL  %s\n" "firmware/tests/native/core_tests.cpp"
    print_prefixed_file "$WORK/err.txt"
    print_prefixed_file "$WORK/out.txt"
    fail=1
  fi
fi

echo
if [ "$fail" -eq 0 ]; then
  echo "host_check: PASS"
else
  echo "host_check: FAIL"
fi
exit "$fail"
