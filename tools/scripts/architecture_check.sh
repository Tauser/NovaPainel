#!/usr/bin/env bash
# Architecture gate for NovaPanel firmware layering.
set -euo pipefail

SCRIPT_PATH="${BASH_SOURCE[0]}"
SCRIPT_DIR="${SCRIPT_PATH%/*}"
[ "$SCRIPT_DIR" = "$SCRIPT_PATH" ] && SCRIPT_DIR="."
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FIRMWARE="$ROOT/firmware"

if [ ! -d "$FIRMWARE" ]; then
  echo "architecture_check: no active firmware, SKIP"
  exit 0
fi

bad=0

line_count() {
  awk 'END { print NR }' "$1"
}

require_line_limit() {
  local path="$1"
  local limit="$2"
  local count
  count="$(line_count "$path")"
  if [ "$count" -gt "$limit" ]; then
    echo "ERROR: $path has $count lines, limit is $limit" >&2
    bad=1
  fi
}

extract_requires() {
  local cmake_file="$1"
  awk '
    BEGIN { in_requires = 0 }
    /^[[:space:]]*REQUIRES[[:space:]]*$/ { in_requires = 1; next }
    in_requires && /^[[:space:]]*\)[[:space:]]*$/ { in_requires = 0 }
    in_requires {
      line = $0
      sub(/#.*/, "", line)
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", line)
      if (line != "") print line
    }
  ' "$cmake_file"
}

assert_requires_subset() {
  local component="$1"
  shift
  local allowed=" $* "
  local cmake_file="$FIRMWARE/components/$component/CMakeLists.txt"

  if [ ! -f "$cmake_file" ]; then
    echo "ERROR: missing CMakeLists for component $component" >&2
    bad=1
    return
  fi

  while IFS= read -r dep; do
    case "$allowed" in
      *" $dep "*) ;;
      *)
        echo "ERROR: component $component has forbidden REQUIRES dependency: $dep" >&2
        bad=1
        ;;
    esac
  done < <(extract_requires "$cmake_file")
}

assert_no_include() {
  local component="$1"
  local pattern="$2"
  if [ -d "$FIRMWARE/components/$component" ] &&
     grep -R -nE "$pattern" "$FIRMWARE/components/$component" >/tmp/novapanel_arch_grep.$$; then
    cat /tmp/novapanel_arch_grep.$$ >&2
    echo "ERROR: component $component includes forbidden layer pattern: $pattern" >&2
    bad=1
  fi
  rm -f /tmp/novapanel_arch_grep.$$
}

require_line_limit "$FIRMWARE/main/app_main.cpp" 300

assert_requires_subset "ui" core lvgl models
assert_requires_subset "providers" models utils
assert_requires_subset "core" models utils

assert_no_include "ui" '"(provider_interfaces|i_board|mock_board|waveshare_board)\.hpp"'
assert_no_include "providers" '"(event_bus|state_store|ui_dispatcher|screen_|i_board|mock_board|waveshare_board)\.hpp"'
assert_no_include "board" '"(event_bus|state_store|ui_dispatcher|screen_|provider_interfaces)\.hpp"'

if [ "$bad" -ne 0 ]; then
  exit 1
fi

echo "architecture_check: PASS"
