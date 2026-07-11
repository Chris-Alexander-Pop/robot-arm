#!/usr/bin/env bash
# Flash ESP32-C3 motor bench nodes (IDs 1–4).
#
# Usage:
#   ./flash_motor_nodes.sh                # interactive
#   ./flash_motor_nodes.sh --all          # flash all without prompts
#   ./flash_motor_nodes.sh --node 3       # flash single node
#
# Prerequisites:
#   - PlatformIO CLI on PATH
#   - src/wifi_config.h present (copy from wifi_config.h.example)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/esp32"

ALL_NODES=(1 2 3 4)
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
            echo "Usage: $0 [--all | --node <1-4>]"
            exit 1
            ;;
    esac
done

if [[ ! -f "$PROJECT_DIR/src/wifi_config.h" ]]; then
    echo "ERROR: $PROJECT_DIR/src/wifi_config.h not found." >&2
    echo "Copy wifi_config.h.example to wifi_config.h and set WiFi credentials." >&2
    exit 1
fi

find_esp_port() {
    local by_id
    by_id="$(ls -1 /dev/serial/by-id/usb-Espressif_* 2>/dev/null | head -n1 || true)"
    if [[ -n "$by_id" ]]; then
        readlink -f "$by_id"
        return 0
    fi
    local port
    for port in /dev/ttyACM{0..9} /dev/ttyUSB{0..9}; do
        [[ -e "$port" ]] || continue
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
    echo "=== Flashing motor node_${node_id} (env: ${env}) ==="
    if port="$(find_esp_port)"; then
        echo "Using ESP upload port: $port"
        pio run -d "$PROJECT_DIR" -e "$env" -t upload --upload-port "$port"
    else
        echo "ERROR: no Espressif serial port found." >&2
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
        if [[ "$SINGLE_NODE" -lt 1 || "$SINGLE_NODE" -gt 4 ]]; then
            echo "ERROR: node must be 1-4" >&2
            exit 1
        fi
        flash_node "$SINGLE_NODE"
        ;;
    interactive)
        echo "=============================================="
        echo " RS-485 motor bench ESP32 batch flash"
        echo "=============================================="
        for n in "${ALL_NODES[@]}"; do
            read -rp ">>> Plug in node_${n} and press Enter... "
            flash_node "$n"
        done
        ;;
esac

echo ""
echo "All done. Flash Pico master, then: ./run_motor_bench.sh"
