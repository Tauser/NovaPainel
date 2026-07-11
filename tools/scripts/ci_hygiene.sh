#!/usr/bin/env bash
# Repository hygiene gate used by the Linux CI workflow.
set -euo pipefail

SCRIPT_PATH="${BASH_SOURCE[0]}"
SCRIPT_DIR="${SCRIPT_PATH%/*}"
[ "$SCRIPT_DIR" = "$SCRIPT_PATH" ] && SCRIPT_DIR="."
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$ROOT"

bad=0
while IFS= read -r path; do
  case "$path" in
    */build/*|*/node_modules/*|*/managed_components/*|*/__pycache__/*|*/sdkconfig|*/sdkconfig.old)
      echo "ERROR: generated artifact tracked: $path" >&2
      bad=1
      ;;
  esac

  case "$path" in
    reference/*) continue ;;
  esac
  if [ "$(git cat-file -s ":$path")" -gt 5242880 ]; then
    echo "ERROR: tracked file over 5 MiB outside reference/: $path" >&2
    bad=1
  fi
done < <(git ls-files)

if git grep -nE '(BEGIN (RSA |EC |OPENSSH )?PRIVATE KEY|AKIA[0-9A-Z]{16})' -- ':!reference/**'; then
  echo "ERROR: possible secret in tracked content" >&2
  bad=1
fi

if [ "$bad" -ne 0 ]; then
  exit 1
fi

echo "ci_hygiene: PASS"
