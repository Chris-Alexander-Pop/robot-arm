#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

if [[ ! -x "$ROOT_DIR/dev.sh" ]]; then
  echo "Error: dev.sh is missing or not executable. Run from the repository root workflow."
  exit 1
fi

"$ROOT_DIR/dev.sh" up

"$ROOT_DIR/dev.sh" moveit-test