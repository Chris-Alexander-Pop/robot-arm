#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

if [[ ! -x "$ROOT_DIR/scripts/dev.sh" ]]; then
  echo "Error: scripts/dev.sh is missing or not executable."
  exit 1
fi

"$ROOT_DIR/scripts/dev.sh" build
