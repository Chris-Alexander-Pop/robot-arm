#!/usr/bin/env bash
# Build, flash, and open monitor for the all-axes sequential jog test.
#
# Required hardware:
#   - Nucleo-F401RE connected via ST-Link USB
#   - All 6 stepper drivers (CL57T / CL42T) powered and wired per pinout.h
#
# Usage:
#   cd firmware/scripts/hardware_tests
#   ./run_stepper_all.sh
#
# Press Ctrl+C to exit the serial monitor.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../../stm32_core" && pwd)"

echo "=== HWTEST: All 6 axes sequential jog ==="
echo "Project: ${PROJECT_DIR}"
echo ""

cd "${PROJECT_DIR}"

echo "[1/2] Building and flashing..."
PLATFORMIO_BUILD_FLAGS="-DHWTEST_STEPPER_ALL_AXES" \
  pio run -e nucleo_f401re_hwtest -t upload

echo ""
echo "[2/2] Opening serial monitor (Ctrl+C to quit)..."
pio device monitor -e nucleo_f401re_hwtest
