#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
RENODE_SCRIPT="$ROOT_DIR/simulation/renode/nucleo_f401re.resc"
LOCAL_RENODE="$ROOT_DIR/.tooling/bin/renode"

if [[ -x "$LOCAL_RENODE" ]]; then
  RENODE_BIN="$LOCAL_RENODE"
elif command -v renode >/dev/null 2>&1; then
  RENODE_BIN="$(command -v renode)"
else
  echo "Error: renode is not installed. Run ./setup.sh first."
  exit 1
fi

cd "$ROOT_DIR"
"$RENODE_BIN" --disable-xwt --console -e "i @$RENODE_SCRIPT; sysbus LoadELF @${ROOT_DIR}/firmware/stm32_core/.pio/build/genericSTM32F401CC_renode/firmware.elf; start"