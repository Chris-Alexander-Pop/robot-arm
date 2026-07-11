// WiFi UDP log shipper for ESP32-C3 bench nodes.
//
// Connects to the bench WiFi AP and fires UDP datagrams to the log hub
// (rs485_log_hub.py) on LOG_HOST_IP:LOG_UDP_PORT.
//
// Log line format:
//   J<id>|<tag>|<message>
//
// The hub prefixes a timestamp and colorises per-node.

#pragma once

#include <stdint.h>
#include <stdarg.h>

// Start WiFi and UDP socket.  Call once from setup().
// Returns true when WiFi is associated and socket is ready.
bool WifiLogBegin(uint8_t node_id);

// Returns true if WiFi is currently connected.
bool WifiLogConnected();

// Printf-style log send.  Silently no-ops only if WiFi is down.
// High-rate RX MOVE/HB lines are coalesced (e.g. "MOVEx12 last=...") and
// batched into multi-line UDP datagrams — no silent rate-limit drops.
// tag: short uppercase tag, e.g. "BOOT", "RX", "ERR"
void WifiLog(const char* tag, const char* fmt, ...);

// Call from loop() — drives WiFi reconnect if the link drops.
void WifiLogPoll();
