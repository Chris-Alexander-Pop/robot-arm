#!/usr/bin/env bash
# Build, flash, and monitor the Arduino RS-485 echo (PlatformIO).
#
# Usage:
#   ./run_rs485_arduino.sh           # defaults to env "uno"
#   ./run_rs485_arduino.sh nano
#   ./run_rs485_arduino.sh mega
#
# Requires: PlatformIO CLI (pio), Arduino on USB

set -euo pipefail

ENV_NAME="${1:-uno}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}/rs485_arduino"

case "${ENV_NAME}" in
  uno|nano|mega) ;;
  *)
    echo "Unknown board env '${ENV_NAME}'. Use: uno, nano, or mega"
    exit 1
    ;;
esac

echo "=== Arduino RS-485 echo (PlatformIO env: ${ENV_NAME}) ==="
echo "Project: ${PROJECT_DIR}"
echo ""

cd "${PROJECT_DIR}"

echo "[1/2] Building and flashing..."
pio run -e "${ENV_NAME}" -t upload

echo ""
echo "[2/2] Serial monitor @ 115200 (Ctrl+C to quit)..."
echo "Bus baud 38400 (see rs485_bench_config.h). Then flash Nucleo: ./run_rs485.sh"
echo ""
pio device monitor -e "${ENV_NAME}"
