#!/usr/bin/env bash
# bench_connect.sh — Connect to the bench hotspot and sync the PC's IP into wifi_config.h
#
# Usage:
#   ./bench_connect.sh            # connect (if needed) and update IP
#   ./bench_connect.sh --status   # just print current state, no changes
#
# Add to your session startup or run before flashing / starting the hub.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WIFI_CONFIG="$SCRIPT_DIR/rs485_esp32_bench/src/wifi_config.h"

SSID="CAP_P9"
IFACE="wlan0"

# ── Parse args ────────────────────────────────────────────────────────────────
STATUS_ONLY=false
if [[ "${1:-}" == "--status" ]]; then
    STATUS_ONLY=true
fi

# ── Check / connect ───────────────────────────────────────────────────────────
current_ssid=$(nmcli -t -f active,ssid dev wifi 2>/dev/null \
    | grep "^yes:" | cut -d: -f2 || true)

if [[ "$current_ssid" != "$SSID" ]]; then
    if $STATUS_ONLY; then
        echo "Not connected to $SSID (current: ${current_ssid:-none})"
        exit 1
    fi
    echo "Connecting to $SSID ..."
    nmcli dev wifi connect "$SSID" ifname "$IFACE"
    sleep 2
fi

# ── Get IP ────────────────────────────────────────────────────────────────────
PC_IP=$(ip addr show "$IFACE" 2>/dev/null \
    | grep "inet " | awk '{print $2}' | cut -d/ -f1 | head -1)

if [[ -z "$PC_IP" ]]; then
    echo "ERROR: connected to $SSID but no IP assigned yet — try again in a few seconds" >&2
    exit 1
fi

echo "Connected: SSID=$SSID  PC_IP=$PC_IP"

if $STATUS_ONLY; then
    current_config_ip=$(grep 'define LOG_HOST_IP' "$WIFI_CONFIG" \
        | grep -oP '"\K[^"]+' || true)
    echo "wifi_config.h LOG_HOST_IP=${current_config_ip:-not set}"
    [[ "$PC_IP" == "$current_config_ip" ]] && echo "(up to date)" || echo "(needs update — run without --status)"
    exit 0
fi

# ── Update wifi_config.h if IP changed ────────────────────────────────────────
current_config_ip=$(grep 'define LOG_HOST_IP' "$WIFI_CONFIG" \
    | grep -oP '"\K[^"]+' || true)

if [[ "$PC_IP" == "$current_config_ip" ]]; then
    echo "wifi_config.h already has LOG_HOST_IP=$PC_IP — no update needed"
else
    echo "Updating LOG_HOST_IP: ${current_config_ip:-unset} → $PC_IP"
    sed -i "s|#define LOG_HOST_IP \"[^\"]*\"|#define LOG_HOST_IP \"$PC_IP\"|" "$WIFI_CONFIG"
    echo "wifi_config.h updated."
    echo ""
    echo "Re-flash any ESP32-C3 nodes whose IP has changed:"
    echo "  cd $(dirname "$SCRIPT_DIR")/hardware_tests"
    echo "  ./flash_rs485_esp32_nodes.sh --node <id>"
fi

echo ""
echo "Hub command:"
echo "  ./run_rs485_log_hub.sh"
