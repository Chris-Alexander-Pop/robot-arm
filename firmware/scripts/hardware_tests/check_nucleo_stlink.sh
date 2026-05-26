#!/usr/bin/env bash
# Verify onboard ST-Link can reach the Nucleo target MCU before pio upload.
# Exit 0 = SWD OK; exit 1 = ST-Link missing; exit 2 = ST-Link present but target unreachable.

set -euo pipefail

expect_chip_name="STM32F401"

echo "=== Nucleo ST-Link diagnostic ==="

if ! lsusb -d 0483:374b >/dev/null 2>&1; then
  echo "FAIL: No ST-Link USB device (0483:374b)."
  echo "      Plug the Nucleo in via the ST-LINK USB connector (not only the Arduino Uno)."
  exit 1
fi

if ! command -v st-info >/dev/null 2>&1; then
  echo "WARN: st-info not found (install package: stlink). Skipping SWD probe."
  exit 0
fi

probe_out="$(st-info --probe 2>&1)" || true
echo "${probe_out}"

chipid="$(echo "${probe_out}" | awk '/chipid:/ {print $2}')"
if [[ "${chipid}" == "0x000" || "${chipid}" == "0x0" ]]; then
  echo ""
  echo "FAIL: ST-Link is connected but cannot talk to the onboard STM32 (SWD)."
fi

nrst_ok=1
if command -v st-flash >/dev/null 2>&1; then
  flash_out="$(st-flash --connect-under-reset --freq=50 read /tmp/.nucleo_stlink_probe.bin 0x08000000 4 2>&1)" || true
  if echo "${flash_out}" | grep -qi 'NRST is not connected'; then
    echo "FAIL: NRST is not connected between ST-Link and the target MCU."
    nrst_ok=0
  fi
fi

mbed_fail=""
mbed_dev="$(lsblk -ndo NAME,LABEL 2>/dev/null | awk '$2 ~ /NOD_F401RE|NODE_F401RE/ {print $1; exit}')"
if [[ -n "${mbed_dev}" ]] && command -v udisksctl >/dev/null 2>&1; then
  mount_msg="$(udisksctl mount -b "/dev/${mbed_dev}" --no-user-interaction 2>&1)" || true
  mnt="$(echo "${mount_msg}" | awk '{print $NF}')"
  if [[ -n "${mnt}" && -f "${mnt}/FAIL.TXT" ]]; then
    mbed_fail="$(tr -d '\r' < "${mnt}/FAIL.TXT")"
    echo "mbed drive: ${mbed_fail}"
    udisksctl unmount -b "/dev/${mbed_dev}" --no-user-interaction >/dev/null 2>&1 || true
  elif [[ -n "${mnt}" ]]; then
    udisksctl unmount -b "/dev/${mbed_dev}" --no-user-interaction >/dev/null 2>&1 || true
  fi
fi

if [[ "${chipid}" != "0x000" && "${chipid}" != "0x0" && -n "${chipid}" ]]; then
  echo "OK: Target MCU reachable (chipid ${chipid})."
  exit 0
fi

echo ""
echo "This is a hardware/debug link problem — firmware build/upload scripts are fine."
echo ""
echo "Fix (check in order):"
echo "  1. CN2 — both jumpers must be ON (links ST-Link to the STM32). Default for"
echo "     onboard programming; if they are off, SWD/NRST never reach the chip."
echo "  2. CN4 — remove any dupont wires on the small debug header; they disturb"
echo "     onboard SWD when CN2 is on (Nucleo-64 UM1724 §6.2.3)."
echo "  3. Cut tab — if the ST-Link section was snapped off the board, CN2 no longer"
echo "     works; wire SWDIO/SWCLK/NRST/GND from CN4 to the target instead."
echo "  4. Bench wiring — nothing on morpho NRST; RS-485 uses D8/D2/A2 only."
echo "  5. Power cycle — unplug ST-LINK USB 10 s, reconnect, then:"
echo "       st-info --probe"
echo "     Expect chipid 0x433 for ${expect_chip_name}."
echo ""
echo "Having an Arduino plugged in is OK; it does not block Nucleo flash if CN2 is correct."
exit 2
