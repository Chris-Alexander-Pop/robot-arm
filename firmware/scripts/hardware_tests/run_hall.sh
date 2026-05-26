#!/usr/bin/env bash
# Build, flash, and monitor the Hall sensor bench test on Nucleo-F401RE.
#
# Wiring: firmware/scripts/hardware_tests/hall-effect-sensor-testing/README.md
#
# Usage:
#   cd firmware/scripts/hardware_tests
#   ./run_hall.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../../stm32_core" && pwd)"

echo "=== HWTEST: Hall sensor (A3144-style) ==="
echo "Project: ${PROJECT_DIR}"
echo "Wiring: hall-effect-sensor-testing/README.md (A5 / PC0)"
echo ""

cd "${PROJECT_DIR}"

echo "[1/2] Building and flashing..."
echo "Tip: Close any other pio device monitor (RS485/Arduino) before flashing the Nucleo."
echo ""

if ! PLATFORMIO_BUILD_FLAGS="-DHWTEST_HALL" \
  pio run -e nucleo_f401re_hwtest -t upload; then
  echo ""
  echo "Upload failed (ST-Link could not connect). Try:"
  echo "  1. Ctrl+C in every terminal running pio device monitor"
  echo "  2. Unplug Nucleo USB, plug back in (ST-Link port only)"
  echo "  3. On the Nucleo PCB: both CN2 jumpers fitted (ST-LINK to target)"
  echo "  4. Press black RESET, run ./run_hall.sh again immediately"
  echo "  5. st-info --probe   (should show chipid 0x433, not 0x000)"
  exit 1
fi

echo ""
echo "[2/2] Serial monitor @ 115200 (Ctrl+C to quit)..."
echo "Bring a magnet near the sensor — expect MAGNET NEAR (HOME)."
echo ""
pio device monitor -e nucleo_f401re_hwtest
