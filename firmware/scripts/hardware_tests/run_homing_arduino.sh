#!/usr/bin/env bash
# Arduino homing test — CL57T + Hall (PlatformIO).
#
# Usage:
#   ./run_homing_arduino.sh uno
#
# Wiring: homing_arduino/README.md

set -euo pipefail

ENV_NAME="${1:-uno}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}/homing_arduino"

case "${ENV_NAME}" in
  uno|nano|mega) ;;
  *)
    echo "Use: uno, nano, or mega"
    exit 1
    ;;
esac

echo "=== Arduino homing test (CL57T + Hall) ==="
echo "Wiring: ${PROJECT_DIR}/README.md"
echo "After flash: type HOME in serial monitor @ 115200"
echo ""

cd "${PROJECT_DIR}"
pio run -e "${ENV_NAME}" -t upload
echo ""
pio device monitor -e "${ENV_NAME}"
