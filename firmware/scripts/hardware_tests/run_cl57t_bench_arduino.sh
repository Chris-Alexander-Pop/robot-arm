#!/usr/bin/env bash
# Arduino CL57T bench — speed sweep forward/reverse (PlatformIO).
#
# Usage:
#   ./run_cl57t_bench_arduino.sh uno
#
# Wiring: cl57t_bench_arduino/README.md
# After flash: type GO @ 115200

set -euo pipefail

ENV_NAME="${1:-uno}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}/cl57t_bench_arduino"

case "${ENV_NAME}" in
  uno|nano|mega) ;;
  *)
    echo "Use: uno, nano, or mega"
    exit 1
    ;;
esac

echo "=== Arduino CL57T bench (fwd/rev speed sweep) ==="
echo "Wiring: ${PROJECT_DIR}/README.md"
echo "After flash: type GO in serial monitor @ 115200"
echo ""

cd "${PROJECT_DIR}"
pio run -e "${ENV_NAME}" -t upload
echo ""
pio device monitor -e "${ENV_NAME}"
