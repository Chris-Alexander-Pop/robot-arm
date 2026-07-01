#!/usr/bin/env bash
# Flash and monitor the RS-485 Pico master bench firmware.
#
# Usage:
#   ./run_rs485_pico.sh [--port <dev>]
#
# Requires:
#   - PlatformIO CLI on PATH  (pip install platformio)
#   - Pico connected via USB (BOOTSEL mode or regular UF2 upload)
#
# The script builds, uploads, then opens the serial monitor at 115200.
# CTRL-C to exit the monitor; the Pico keeps running.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/rs485_pico"
EXTRA_PORT_ARGS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --port)
            EXTRA_PORT_ARGS=(--upload-port "$2")
            shift 2
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

echo "=== Building rs485_pico ==="
pio run -d "$PROJECT_DIR" -e pico "${EXTRA_PORT_ARGS[@]:+${EXTRA_PORT_ARGS[@]}}" -t upload

echo ""
echo "=== Opening serial monitor (115200) — Ctrl-] to quit ==="
python3 -m serial.tools.miniterm /dev/ttyACM0 115200 --eol CRLF
