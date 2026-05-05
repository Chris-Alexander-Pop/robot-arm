#!/usr/bin/env bash
# Build, flash, and open monitor for the single-axis stepper hardware test.
#
# Required hardware:
#   - Nucleo-F401RE connected via ST-Link USB
#   - NEMA 23 motor wired to CL57T, STEP→kJ1StepPin, DIR→kJ1DirPin
#   - CL57T powered (24-48 V)
#
# Usage:
#   cd firmware/scripts/hardware_tests
#   ./run_stepper_single.sh
#
# Press Ctrl+C to exit the serial monitor.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../../stm32_core" && pwd)"

echo "=== HWTEST: Single-axis stepper (NEMA 23 + CL57T) ==="
echo "Project: ${PROJECT_DIR}"
echo ""

cd "${PROJECT_DIR}"

echo "[1/2] Building and flashing..."
PLATFORMIO_BUILD_FLAGS="-DHWTEST_STEPPER_SINGLE" \
  pio run -e nucleo_f401re_hwtest -t upload

echo ""
echo "[2/2] Opening serial monitor (Ctrl+C to quit)..."
pio device monitor -e nucleo_f401re_hwtest
