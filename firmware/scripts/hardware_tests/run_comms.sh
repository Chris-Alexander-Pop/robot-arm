#!/usr/bin/env bash
# Build, flash, and open monitor for the serial protocol loopback test.
#
# Required hardware:
#   - Nucleo-F401RE connected via ST-Link USB
#   - A host (PC or Raspberry Pi) with serial access at 115200 baud
#   - The robot_arm host software (or a manual serial sender) to send
#     JointCommand and heartbeat packets
#
# Packet format (for manual testing):
#   Heartbeat:    AA 55 12 <xor of 0x12>         = AA 55 12 12
#   JointCommand: AA 55 10 [6x float32 LE] <xor>  (28 bytes total)
#
# Usage:
#   cd firmware/scripts/hardware_tests
#   ./run_comms.sh
#
# Press Ctrl+C to exit the serial monitor.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../../stm32_core" && pwd)"

echo "=== HWTEST: Serial protocol loopback ==="
echo "Project: ${PROJECT_DIR}"
echo ""

cd "${PROJECT_DIR}"

echo "[1/2] Building and flashing..."
PLATFORMIO_BUILD_FLAGS="-DHWTEST_COMMS" \
  pio run -e nucleo_f401re_hwtest -t upload

echo ""
echo "[2/2] Opening serial monitor (Ctrl+C to quit)..."
echo "Send JointCommand (0xAA 0x55 0x10 ...) or Heartbeat (0xAA 0x55 0x12 0x12)"
echo "packets from the host.  The board will echo decoded values back."
echo ""
pio device monitor -e nucleo_f401re_hwtest
