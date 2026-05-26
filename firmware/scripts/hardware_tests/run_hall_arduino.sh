#!/usr/bin/env bash
# Build, flash, and monitor Arduino Hall sensor test (A3144 on A0).
#
# Usage:
#   ./run_hall_arduino.sh        # uno (default)
#   ./run_hall_arduino.sh nano

set -euo pipefail

ENV_NAME="${1:-uno}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}/hall_arduino"

case "${ENV_NAME}" in
  uno|nano|mega) ;;
  *)
    echo "Use: uno, nano, or mega"
    exit 1
    ;;
esac

echo "=== Arduino Hall sensor test (env: ${ENV_NAME}) ==="
echo "Wiring: hall_arduino/README.md  (OUT -> A0, VCC -> 5V, GND -> GND)"
echo ""

cd "${PROJECT_DIR}"

pio run -e "${ENV_NAME}" -t upload
echo ""
pio device monitor -e "${ENV_NAME}"
