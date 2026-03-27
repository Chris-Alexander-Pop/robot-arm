#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PROJECT_DIR="$ROOT_DIR/firmware/stm32_core"

if ! command -v pio >/dev/null 2>&1; then
  echo "Error: PlatformIO CLI (pio) is not installed or not in PATH."
  echo "Install the PlatformIO extension in VS Code or PlatformIO Core first."
  exit 1
fi

cd "$PROJECT_DIR"
pio run -e genericSTM32F401CC