#!/usr/bin/env bash
# Start the RS-485 motor bench log hub.
#
# Sources rs485_bench.env from the parent hardware_tests directory if present.
#
# Usage:
#   ./run_motor_bench.sh                    # UDP-only (safe while flashing ESP32s)
#   ./run_motor_bench.sh --with-pico        # also open Pico serial for commands
#   ./run_motor_bench.sh --pico-port /dev/ttyACM0
#   ./run_motor_bench.sh --flash-pico       # build/upload Pico master first
#
# Set PICO_PORT in rs485_bench.env to auto-open Pico without --with-pico.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HW_TESTS_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ENV_FILE="$HW_TESTS_DIR/rs485_bench.env"
HUB_SCRIPT="$HW_TESTS_DIR/rs485_log_hub.py"
PICO_DIR="$SCRIPT_DIR/pico"

FLASH_PICO=0
WITH_PICO=""
HUB_ARGS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --flash-pico)
            FLASH_PICO=1
            shift
            ;;
        --with-pico)
            WITH_PICO=1
            shift
            ;;
        --no-pico)
            WITH_PICO=0
            shift
            ;;
        *)
            HUB_ARGS+=("$1")
            shift
            ;;
    esac
done

if [[ -f "$ENV_FILE" ]]; then
    echo "=== Sourcing $ENV_FILE ==="
    set -a
    # shellcheck source=/dev/null
    source "$ENV_FILE"
    set +a
fi

if [[ "$FLASH_PICO" -eq 1 ]]; then
    echo "=== Flashing Pico motor master ==="
    pio run -d "$PICO_DIR" -e pico -t upload
fi

if [[ ${#HUB_ARGS[@]} -eq 0 ]]; then
    if [[ "$WITH_PICO" == "1" ]]; then
        : # hub auto-detects Pico (skips Espressif)
    elif [[ "$WITH_PICO" == "0" ]]; then
        HUB_ARGS=(--no-pico)
    elif [[ -n "${PICO_PORT:-}" ]]; then
        HUB_ARGS=(--pico-port "$PICO_PORT")
    else
        # Default while flashing ESP32 nodes: UDP logs only. Opening the first
        # ttyACM* often hits an ESP32 USB port and spams the terminal.
        HUB_ARGS=(--no-pico)
    fi
fi

if ! command -v python3 &>/dev/null; then
    echo "ERROR: python3 not found on PATH" >&2
    exit 1
fi

echo "=== Starting rs485_log_hub.py (motor bench mode) ==="
exec python3 "$HUB_SCRIPT" "${HUB_ARGS[@]}"
