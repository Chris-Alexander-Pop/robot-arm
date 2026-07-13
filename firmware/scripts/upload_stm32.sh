#!/usr/bin/env bash
# Build and flash the main STM32 firmware to a Nucleo-F401RE (onboard ST-Link).
#
# Project: firmware/stm32_core
# PlatformIO env: nucleo_f401re  (src/main.cpp — not hardware tests)
#
# Hardware tests use nucleo_f401re_hwtest instead; see firmware/scripts/hardware_tests/.
#
# Usage (from repo root or this directory):
#   ./firmware/scripts/upload_stm32.sh
#   ./firmware/scripts/upload_stm32.sh monitor    # upload, then serial monitor
#   ./firmware/scripts/upload_stm32.sh --skip-check
#
# Requires: PlatformIO CLI (pio), Nucleo ST-LINK USB, CN2 jumpers ON.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
STM32_DIR="${ROOT_DIR}/firmware/stm32_core"
CHECK_SCRIPT="${SCRIPT_DIR}/hardware_tests/check_nucleo_stlink.sh"
PIO_ENV="nucleo_f401re"

OPEN_MONITOR=0
SKIP_CHECK=0

for arg in "$@"; do
  case "${arg}" in
    monitor|--monitor|-m)
      OPEN_MONITOR=1
      ;;
    --skip-check)
      SKIP_CHECK=1
      ;;
    -h|--help)
      sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *)
      echo "Unknown argument: ${arg}" >&2
      echo "Usage: $0 [monitor] [--skip-check]" >&2
      exit 1
      ;;
  esac
done

if ! command -v pio >/dev/null 2>&1; then
  echo "Error: PlatformIO CLI (pio) is not installed or not in PATH." >&2
  exit 1
fi

echo "=== STM32 main firmware (Nucleo-F401RE) ==="
echo "Project: ${STM32_DIR}"
echo "Env:     ${PIO_ENV}"
echo ""

if [[ "${SKIP_CHECK}" -eq 0 ]]; then
  if ! "${CHECK_SCRIPT}"; then
    exit 1
  fi
  echo ""
fi

cd "${STM32_DIR}"

echo "[1/1] Building and flashing via ST-Link..."
pio run -e "${PIO_ENV}" -t upload

echo ""
echo "Upload complete."

if [[ "${OPEN_MONITOR}" -eq 1 ]]; then
  echo "Opening serial monitor (115200 baud, Ctrl+C to quit)..."
  pio device monitor -e "${PIO_ENV}"
fi
