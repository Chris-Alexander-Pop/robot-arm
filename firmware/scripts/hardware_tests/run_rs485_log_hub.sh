#!/usr/bin/env bash
# Start the RS-485 multi-node bench log hub.
#
# Sources rs485_bench.env if present, then launches rs485_log_hub.py.
# The hub listens for UDP logs from ESP32-C3 nodes and forwards commands
# to the Pico master over USB serial.
#
# Usage:
#   ./run_rs485_log_hub.sh                    # auto-detect Pico (skips Espressif)
#   ./run_rs485_log_hub.sh --no-pico          # UDP logs only (use while flashing ESP32s)
#   ./run_rs485_log_hub.sh --pico-port /dev/ttyACM0
#
# Requires Python 3 (stdlib only). For Pico serial support:
#   pip install pyserial

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="$SCRIPT_DIR/rs485_bench.env"
HUB_SCRIPT="$SCRIPT_DIR/rs485_log_hub.py"

if [[ -f "$ENV_FILE" ]]; then
    echo "=== Sourcing $ENV_FILE ==="
    # shellcheck source=/dev/null
    set -a; source "$ENV_FILE"; set +a
fi

HUB_ARGS=("$@")
if [[ ${#HUB_ARGS[@]} -eq 0 && -n "${PICO_PORT:-}" ]]; then
    HUB_ARGS=(--pico-port "$PICO_PORT")
fi

# Check Python 3
if ! command -v python3 &>/dev/null; then
    echo "ERROR: python3 not found on PATH" >&2
    exit 1
fi

echo "=== Starting rs485_log_hub.py ==="
exec python3 "$HUB_SCRIPT" "${HUB_ARGS[@]}"
