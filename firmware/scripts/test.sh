#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

if ! command -v pio >/dev/null 2>&1; then
  echo "Error: PlatformIO CLI (pio) is not installed or not in PATH."
  exit 1
fi

cd "$ROOT_DIR/firmware/stm32_core"
pio run -e native

if [[ -x .pio/build/native/program ]]; then
  ./.pio/build/native/program
elif [[ -x .pio/build/native/program.exe ]]; then
  ./.pio/build/native/program.exe
else
  echo "Error: native test program was not built."
  exit 1
fi