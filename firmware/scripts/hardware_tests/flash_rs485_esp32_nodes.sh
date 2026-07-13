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

find_esp_port() {
    # Prefer Espressif by-id symlink so we never accidentally flash the Pico
    # when both are plugged in (pio auto-detect often picks ttyACM0 = Pico).
    local by_id
    by_id="$(ls -1 /dev/serial/by-id/usb-Espressif_* 2>/dev/null | head -n1 || true)"
    if [[ -n "$by_id" ]]; then
        readlink -f "$by_id"
        return 0
    fi
    # Fallback: first ACM/USB that is not a Pico by-id
    local port
    for port in /dev/ttyACM{0..9} /dev/ttyUSB{0..9}; do
        [[ -e "$port" ]] || continue
        if ls -1 /dev/serial/by-id/*Pico*"$(basename "$port")" &>/dev/null; then
            continue
        fi
        # If a Pico by-id points at this port, skip it
        local pico
        for pico in /dev/serial/by-id/*Pico*; do
            [[ -e "$pico" ]] || continue
            if [[ "$(readlink -f "$pico")" == "$(readlink -f "$port")" ]]; then
                continue 2
            fi
        done
        echo "$port"
        return 0
    done
    return 1
}

flash_node() {
    local node_id="$1"
    local env="node_${node_id}"
    local port=""
    echo ""
    echo "=== Flashing node_${node_id} (env: ${env}) ==="
    if port="$(find_esp_port)"; then
        echo "Using ESP upload port: $port"
        pio run -d "$PROJECT_DIR" -e "$env" -t upload --upload-port "$port"
    else
        echo "ERROR: no Espressif serial port found. Plug in the ESP32-C3 USB." >&2
        return 1
    fi
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
