#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$ROOT_DIR"

echo "=== Robot Arm Team Setup (Shell) ==="

if ! command -v python3 >/dev/null 2>&1; then
  echo "Error: python3 is not installed or not in PATH."
  exit 1
fi

if ! command -v docker >/dev/null 2>&1; then
  echo "Warning: docker is not installed or not in PATH. ROS container workflow will not work yet."
fi

echo "[1/4] Creating simulation virtual environment"
bash "$ROOT_DIR/scripts/create-venv.sh"

echo "[2/4] Creating local ROS tooling environment (.tooling)"
python3 -m venv "$ROOT_DIR/.tooling"
# shellcheck disable=SC1090
source "$ROOT_DIR/.tooling/bin/activate"
python -m pip install --upgrade pip
python -m pip install colcon-common-extensions vcstool rosdep

echo "[3/4] Installing Renode"
if command -v brew >/dev/null 2>&1 && [[ "$(uname -s)" == "Darwin" ]]; then
  if brew list renode >/dev/null 2>&1; then
    echo "Renode is already installed via Homebrew."
  else
    brew install renode
  fi
else
  python3 "$ROOT_DIR/scripts/install-renode.py" --root "$ROOT_DIR"
fi

echo "[4/4] Verifying core commands"
colcon list --base-paths "$ROOT_DIR/software/ros2_ws/src" || true

echo "Setup complete."
echo "Use './dev.sh up' to start ROS container and './dev.sh build' to build ROS packages."
