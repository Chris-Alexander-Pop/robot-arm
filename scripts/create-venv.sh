#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SIM_DIR="$ROOT_DIR/simulation"
VENV_DIR="$SIM_DIR/.venv"

if ! command -v python3 >/dev/null 2>&1; then
  echo "Error: python3 is not installed or not in PATH."
  exit 1
fi

echo "Creating simulation virtual environment at $VENV_DIR"
python3 -m venv "$VENV_DIR"

# shellcheck disable=SC1090
source "$VENV_DIR/bin/activate"
python -m pip install --upgrade pip
python -m pip install -r "$SIM_DIR/requirements.txt"

echo "Simulation virtual environment is ready."
echo "Activate with: source simulation/.venv/bin/activate"
