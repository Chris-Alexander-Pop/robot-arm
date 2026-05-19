#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VENV_ACTIVATE="$ROOT_DIR/simulation/.venv/bin/activate"

if [[ ! -f "$VENV_ACTIVATE" ]]; then
  echo "Error: simulation virtual environment not found. Run ./scripts/setup.sh first."
  exit 1
fi

# shellcheck disable=SC1090
source "$VENV_ACTIVATE"
cd "$ROOT_DIR/simulation"
pytest