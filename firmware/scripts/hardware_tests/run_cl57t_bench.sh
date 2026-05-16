#!/usr/bin/env bash
# Build, flash, and open monitor for the CL57T bench hardware test (PA0/PA1).
#
# Required hardware:
#   - Nucleo-F401RE via ST-Link USB
#   - PA0 -> 74HCT541 A0 -> Y0 -> CL57T PUL+
#   - PA1 -> 74HCT541 A1 -> Y1 -> CL57T DIR+
#   - CL57T PUL-, DIR-, COM + 541 GND common; 541 /OE1, /OE2 -> GND
#   - CL57T S3 = 5 V; motor PSU on P4; encoder on P2
#
# Usage:
#   cd firmware/scripts/hardware_tests
#   ./run_cl57t_bench.sh
#
# Press Ctrl+C to exit the serial monitor.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../../stm32_core" && pwd)"

echo "=== HWTEST: CL57T bench (PA0=STEP, PA1=DIR, low speed) ==="
echo "Project: ${PROJECT_DIR}"
echo ""
echo "Ensure the shaft can rotate freely. Press B1 (blue USER button) to start motion."
echo ""

cd "${PROJECT_DIR}"

echo "[1/2] Building and flashing..."
PLATFORMIO_BUILD_FLAGS="-DHWTEST_CL57T_BENCH" \
  pio run -e nucleo_f401re_hwtest -t upload

echo ""
echo "[2/2] Opening serial monitor (115200 baud, Ctrl+C to quit)..."
pio device monitor -e nucleo_f401re_hwtest
