#!/usr/bin/env bash
# Start the RS-485 multi-node bench log hub.
#
# Sources rs485_bench.env if present, then launches rs485_log_hub.py.
# The hub listens for UDP logs from ESP32-C3 nodes and forwards commands
# to the Pico master over USB serial.
#
# Usage:
#   ./run_rs485_log_hub.sh [--pico-port /dev/ttyACM0] [--udp-port 9000]
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

# Check Python 3
if ! command -v python3 &>/dev/null; then
    echo "ERROR: python3 not found on PATH" >&2
    exit 1
fi

echo "=== Starting rs485_log_hub.py ==="
exec python3 "$HUB_SCRIPT" "$@"
