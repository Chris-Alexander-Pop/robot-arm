#!/usr/bin/env bash
# Build, flash, and open monitor for the RS-485 transceiver test.
#
# Required hardware:
#   - Nucleo-F401RE connected via ST-Link USB
#   - MAX485 on Nucleo D8/D2/A2 + 3.3V/GND (see src/pinout.h)
#   - Arduino flashed with rs485_arduino/rs485_arduino.ino; A-A, B-B, GND-GND
#
# Usage:
#   cd firmware/scripts/hardware_tests
#   ./run_rs485.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../../stm32_core" && pwd)"

echo "=== HWTEST: RS-485 transceiver ==="
echo "Project: ${PROJECT_DIR}"
echo ""

cd "${PROJECT_DIR}"

echo "Before flashing: wire bus, common GND, run ./run_rs485_arduino.sh (PlatformIO)."
echo ""

echo "[1/2] Building and flashing Nucleo..."
PLATFORMIO_BUILD_FLAGS="-DHWTEST_RS485" \
  pio run -e nucleo_f401re_hwtest -t upload

echo ""
echo "[2/2] Opening serial monitor (Ctrl+C to quit)..."
echo "Expect TX pings every second; RX (12 OK) when echo is clean (bus baud 38400)."
echo ""
pio device monitor -e nucleo_f401re_hwtest
