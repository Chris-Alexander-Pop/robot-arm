#!/usr/bin/env bash
# Flash all five ESP32-C3 RS-485 bench nodes.
#
# Plug each node's USB cable in turn and press Enter when prompted.
# The script builds the per-node firmware (baking the node ID) and uploads it.
#
# Prerequisites:
#   - PlatformIO CLI on PATH  (pip install platformio)
#   - ESP32-C3 boards connected via USB (one at a time is safest)
#
# Usage:
#   ./flash_rs485_esp32_nodes.sh                # interactive, prompt per node
#   ./flash_rs485_esp32_nodes.sh --all          # flash all envs without prompts
#   ./flash_rs485_esp32_nodes.sh --node 3       # flash a single node
#
# Tip: confirm each node's WiFi config (src/wifi_config.h) is set before flashing.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/rs485_esp32_bench"

ALL_NODES=(1 2 3 4 5)
MODE="interactive"
SINGLE_NODE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --all)
            MODE="all"
            shift
            ;;
        --node)
            MODE="single"
            SINGLE_NODE="$2"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1" >&2
            echo "Usage: $0 [--all | --node <1-5>]"
            exit 1
            ;;
    esac
done

flash_node() {
    local node_id="$1"
    local env="node_${node_id}"
    echo ""
    echo "=== Flashing node_${node_id} (env: ${env}) ==="
    pio run -d "$PROJECT_DIR" -e "$env" -t upload
    echo "=== node_${node_id} done ==="
}

case "$MODE" in
    all)
        for n in "${ALL_NODES[@]}"; do
            flash_node "$n"
        done
        ;;
    single)
        if [[ "$SINGLE_NODE" -lt 1 || "$SINGLE_NODE" -gt 5 ]]; then
            echo "ERROR: node must be 1-5" >&2
            exit 1
        fi
        flash_node "$SINGLE_NODE"
        ;;
    interactive)
        echo "=============================================="
        echo " RS-485 ESP32-C3 batch flash (interactive)"
        echo "=============================================="
        echo "Connect each node's USB cable in turn."
        echo "Press Enter when the node is connected and ready to flash."
        echo "Press Ctrl-C to abort at any time."
        echo ""
        for n in "${ALL_NODES[@]}"; do
            read -rp ">>> Plug in node_${n} and press Enter (or Ctrl-C to skip)... "
            flash_node "$n"
        done
        ;;
esac

echo ""
echo "All done. Verify nodes with: ./run_rs485_log_hub.sh"
