#!/usr/bin/env bash
# Portable host pre-flight for the NovaPanel v4 firmware.
set -euo pipefail

SCRIPT_PATH="${BASH_SOURCE[0]}"
SCRIPT_DIR="${SCRIPT_PATH%/*}"
[ "$SCRIPT_DIR" = "$SCRIPT_PATH" ] && SCRIPT_DIR="."
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FIRMWARE="$ROOT/firmware"
CHECK_APP=0
CHECK_TESTS=0

usage() {
  echo "Usage: bash tools/scripts/host_check.sh [--app] [--tests]"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --app) CHECK_APP=1 ;;
    --tests) CHECK_TESTS=1 ;;
    *) usage >&2; exit 2 ;;
  esac
  shift
done

echo "NovaPanel host_check"
echo "  root: $ROOT"

# Fase 0 deliberately has no active firmware. Keeping this green lets CI
# validate the repository/tooling before Fase 1 creates compilable sources.
if [ ! -d "$FIRMWARE" ]; then
  echo "  no active firmware (Fase 0): PASS"
  exit 0
fi

if ! command -v "${CXX:-g++}" >/dev/null 2>&1; then
  echo "ERROR: ${CXX:-g++} not found" >&2
  exit 1
fi

CXX="${CXX:-g++}"
STD="${CXXSTD:-c++17}"
MANIFEST="$FIRMWARE/tests/host_sources.txt"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/novapanel-host-check.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
SHIM="$WORK/shim"
mkdir -p "$SHIM"

cat > "$SHIM/esp_log.h" <<'EOF'
#pragma once
#define ESP_LOGD(...) do {} while (0)
#define ESP_LOGI(...) do {} while (0)
#define ESP_LOGW(...) do {} while (0)
#define ESP_LOGE(...) do {} while (0)
EOF

cat > "$SHIM/esp_timer.h" <<'EOF'
#pragma once
#include <cstdint>
extern "C" int64_t esp_timer_get_time(void);
EOF

INCLUDES=("-I$SHIM")
for include_dir in "$FIRMWARE"/components/*/include; do
  [ -d "$include_dir" ] && INCLUDES+=("-I$include_dir")
done

if [ -f "$MANIFEST" ]; then
  while IFS= read -r relative || [ -n "$relative" ]; do
    case "$relative" in ''|'#'*) continue ;; esac
    source_file="$FIRMWARE/$relative"
    if [ ! -f "$source_file" ]; then
      echo "ERROR: missing host source: $relative" >&2
      exit 1
    fi
    object="$WORK/$(basename "${relative%.*}").o"
    "$CXX" -std="$STD" -Wall -Wextra -Werror "${INCLUDES[@]}" \
      -c "$source_file" -o "$object"
    echo "  OK  firmware/$relative"
  done < "$MANIFEST"
else
  echo "  no host source manifest yet"
fi

if [ "$CHECK_APP" -eq 1 ] && [ -f "$FIRMWARE/main/app_main.cpp" ]; then
  "$CXX" -std="$STD" -Wall -Wextra -Werror "${INCLUDES[@]}" \
    -fsyntax-only "$FIRMWARE/main/app_main.cpp"
  echo "  OK  firmware/main/app_main.cpp"
fi

if [ "$CHECK_TESTS" -eq 1 ] && [ -x "$FIRMWARE/tests/native/run_host_tests.sh" ]; then
  "$FIRMWARE/tests/native/run_host_tests.sh"
fi

echo "host_check: PASS"
