#!/usr/bin/env bash
# Render every *.yml harness in hardware/wireviz/ to hardware/wireviz/out/
# Default formats: HTML + SVG. Pass -f to override (see `wireviz --help`).
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WV_DIR="$ROOT_DIR/hardware/wireviz"
OUT_DIR="$WV_DIR/out"
VENV_DIR="$WV_DIR/.venv"
WIREVIZ="$VENV_DIR/bin/wireviz"
COMMON_YAML="$WV_DIR/_common.yml"

FORMATS="hs"
CLEAN=0
QUIET=0

usage() {
  cat <<EOF
Usage: $(basename "$0") [-f FORMATS] [-c] [-q] [FILE ...]

  -f FORMATS   wireviz -f string (default: hs = HTML + SVG; add p for PNG, t for TSV)
  -c           Wipe hardware/wireviz/out/ before rendering
  -q           Suppress per-file wireviz output (just print [ok]/[FAIL])
  FILE ...     Optional list of YAML files (paths or basenames). Default: every
               *.yml in hardware/wireviz/ except leading-underscore helpers.

Examples:
  $(basename "$0")                          # render all, HTML+SVG
  $(basename "$0") -c -f hsp                # clean, then render HTML+SVG+PNG
  $(basename "$0") 30_joint_J1_closed_loop  # one file by basename
EOF
}

while getopts ":f:cqh" opt; do
  case "$opt" in
    f) FORMATS="$OPTARG" ;;
    c) CLEAN=1 ;;
    q) QUIET=1 ;;
    h) usage; exit 0 ;;
    \?) echo "Unknown option: -$OPTARG" >&2; usage; exit 2 ;;
    :)  echo "Option -$OPTARG requires an argument." >&2; exit 2 ;;
  esac
done
shift $((OPTIND - 1))

if [[ ! -x "$WIREVIZ" ]]; then
  echo "Error: wireviz not found at $WIREVIZ" >&2
  echo "       Run ./scripts/create-wireviz-venv.sh first." >&2
  exit 1
fi

if ! command -v dot >/dev/null 2>&1; then
  echo "Error: graphviz 'dot' is not on PATH. Install graphviz and retry." >&2
  exit 1
fi

mkdir -p "$OUT_DIR"
if [[ $CLEAN -eq 1 ]]; then
  echo "Cleaning $OUT_DIR"
  rm -rf "${OUT_DIR:?}"/*
fi

if [[ $# -gt 0 ]]; then
  TARGETS=()
  for arg in "$@"; do
    if [[ -f "$arg" ]]; then
      TARGETS+=("$arg")
    elif [[ -f "$WV_DIR/$arg" ]]; then
      TARGETS+=("$WV_DIR/$arg")
    elif [[ -f "$WV_DIR/${arg}.yml" ]]; then
      TARGETS+=("$WV_DIR/${arg}.yml")
    else
      echo "Skip (not found): $arg" >&2
    fi
  done
else
  shopt -s nullglob
  TARGETS=()
  for f in "$WV_DIR"/*.yml; do
    [[ "$(basename "$f")" == _* ]] && continue
    TARGETS+=("$f")
  done
  shopt -u nullglob
fi

PREPEND_ARGS=()
if [[ -f "$COMMON_YAML" ]]; then
  PREPEND_ARGS=(-p "$COMMON_YAML")
fi

if [[ ${#TARGETS[@]} -eq 0 ]]; then
  echo "No YAML files to render." >&2
  exit 1
fi

fail=0
for src in "${TARGETS[@]}"; do
  base="$(basename "$src" .yml)"
  if [[ $QUIET -eq 1 ]]; then
    if "$WIREVIZ" "${PREPEND_ARGS[@]}" "$src" -o "$OUT_DIR" -O "$base" -f "$FORMATS" >/dev/null 2>&1; then
      echo "[ok]   $base"
    else
      echo "[FAIL] $base"
      fail=$((fail + 1))
    fi
  else
    echo "=== $base ==="
    if ! "$WIREVIZ" "${PREPEND_ARGS[@]}" "$src" -o "$OUT_DIR" -O "$base" -f "$FORMATS"; then
      fail=$((fail + 1))
    fi
  fi
done

echo
if [[ $fail -eq 0 ]]; then
  echo "Rendered ${#TARGETS[@]} file(s) into $OUT_DIR"
else
  echo "Rendered with $fail failure(s); see output above." >&2
  exit 1
fi
