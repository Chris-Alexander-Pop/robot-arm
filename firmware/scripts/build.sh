#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
STM32_DIR="$ROOT_DIR/firmware/stm32_core"
JOINT_DIR="$ROOT_DIR/firmware/joint_node"

if ! command -v pio >/dev/null 2>&1; then
  echo "Error: PlatformIO CLI (pio) is not installed or not in PATH."
  exit 1
fi

build_stm32() {
  echo "==> firmware/stm32_core (nucleo_f401re)"
  (cd "$STM32_DIR" && pio run -e nucleo_f401re)
}

build_joint_default() {
  echo "==> firmware/joint_node (esp32dev)"
  (cd "$JOINT_DIR" && pio run -e esp32dev)
}

build_joint_all() {
  echo "==> firmware/joint_node (all node_* envs)"
  (cd "$JOINT_DIR" && pio run -e node_j1 -e node_j2 -e node_j3 -e node_j4 -e node_j5 -e node_j6 -e node_gripper)
}

TARGET="${1:-all}"

case "$TARGET" in
  all)
    build_stm32
    build_joint_default
    ;;
  stm32)
    build_stm32
    ;;
  joint)
    build_joint_default
    ;;
  joint-all)
    build_joint_all
    ;;
  *)
    echo "Usage: $0 [all|stm32|joint|joint-all]"
    exit 1
    ;;
esac
