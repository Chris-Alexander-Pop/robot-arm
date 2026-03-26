#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

if ! command -v pio >/dev/null 2>&1; then
  echo "Error: PlatformIO CLI (pio) is not installed or not in PATH."
  exit 1
fi

cd "$ROOT_DIR/firmware/stm32_core"
pio test -e native