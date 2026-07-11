#!/usr/bin/env bash
set -euo pipefail

SCRIPT_PATH="${BASH_SOURCE[0]}"
SCRIPT_DIR="${SCRIPT_PATH%/*}"
[ "$SCRIPT_DIR" = "$SCRIPT_PATH" ] && SCRIPT_DIR="."
FIRMWARE="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/novapanel-native-tests.XXXXXX")"
trap 'rm -rf "$BUILD_DIR"' EXIT

CXX="${CXX:-g++}"
INCLUDES=()
for include_dir in "$FIRMWARE"/components/*/include; do
  [ -d "$include_dir" ] && INCLUDES+=("-I$include_dir")
done

SOURCES=()
while IFS= read -r relative || [ -n "$relative" ]; do
  case "$relative" in ''|'#'*) continue ;; esac
  SOURCES+=("$FIRMWARE/$relative")
done < "$FIRMWARE/tests/host_sources.txt"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INCLUDES[@]}" \
  "$FIRMWARE/tests/native/test_core.cpp" "${SOURCES[@]}" -o "$BUILD_DIR/test_core"
"$BUILD_DIR/test_core"
echo "native tests: PASS"
