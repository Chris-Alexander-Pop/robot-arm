#include "wifi_log.h"
#include "wifi_config.h"
#include "rs485_bench_config.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <stdarg.h>
#include <stdio.h>

namespace {

static WiFiUDP g_udp;
static uint8_t g_node_id = 0;
static bool    g_udp_ready = false;

// Rate-limiter: track send timestamp to avoid flooding.
static uint32_t g_last_send_ms = 0;
// 20 lines/s max = 50 ms minimum spacing
constexpr uint32_t kMinSendIntervalMs = 50U;

static void TrySendPacket(const char* line) {
    if (!g_udp_ready) return;
    const uint32_t now = millis();
    if (now - g_last_send_ms < kMinSendIntervalMs) return;
    g_last_send_ms = now;
    g_udp.beginPacket(LOG_HOST_IP, LOG_UDP_PORT);
    g_udp.write(reinterpret_cast<const uint8_t*>(line),
                static_cast<size_t>(strlen(line)));
    g_udp.endPacket();
}

}  // namespace

bool WifiLogBegin(uint8_t node_id) {
    g_node_id = node_id;
    g_udp_ready = false;

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print(F("[wifi] connecting to " WIFI_SSID " "));
    const uint32_t deadline = millis() + 15000U;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[wifi] failed — logs via USB only"));
        return false;
    }

    Serial.print(F("[wifi] connected, IP="));
    Serial.println(WiFi.localIP());
    Serial.print(F("[wifi] log hub: " LOG_HOST_IP ":"));
    Serial.println(LOG_UDP_PORT);

    g_udp.begin(0);  // bind to any local port
    g_udp_ready = true;

    // Send boot line
    char boot_msg[80];
    snprintf(boot_msg, sizeof(boot_msg),
             "J%u|BOOT|node=%u baud=%lu ip=%s",
             g_node_id, g_node_id,
             static_cast<unsigned long>(RS485_BENCH_BAUD),
             WiFi.localIP().toString().c_str());
    TrySendPacket(boot_msg);

    return true;
}

bool WifiLogConnected() {
    return WiFi.status() == WL_CONNECTED && g_udp_ready;
}

void WifiLog(const char* tag, const char* fmt, ...) {
    char msg[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // Also print to USB serial for bench visibility
    Serial.print(F("[J"));
    Serial.print(g_node_id);
    Serial.print('|');
    Serial.print(tag);
    Serial.print(F("] "));
    Serial.println(msg);

    if (!g_udp_ready) return;
    char line[160];
    snprintf(line, sizeof(line), "J%u|%s|%s", g_node_id, tag, msg);
    TrySendPacket(line);
}

void WifiLogPoll() {
    if (g_udp_ready && WiFi.status() != WL_CONNECTED) {
        g_udp_ready = false;
        Serial.println(F("[wifi] connection lost — reconnecting..."));
    }
    if (!g_udp_ready && WiFi.status() == WL_CONNECTED) {
        g_udp.begin(0);
        g_udp_ready = true;
        Serial.println(F("[wifi] reconnected"));
    }
    // Passive reconnect is handled by the ESP-IDF WiFi stack automatically.
}
