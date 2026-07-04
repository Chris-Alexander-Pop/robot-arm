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

    // Print disconnect reason for diagnostics.
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.print(F("[wifi] disconnect reason="));
        Serial.println(info.wifi_sta_disconnected.reason);
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    WiFi.persistent(false);  // skip NVS read/write entirely
    WiFi.disconnect(true);   // clear any residual state
    delay(200);
    WiFi.mode(WIFI_STA);
    delay(200);
    WiFi.setSleep(WIFI_PS_NONE);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    // ESP32-C3 SuperMini: the onboard LDO (250 mA peak) cannot sustain the
    // default ~19.5 dBm TX burst during WPA2 4-way handshake, causing the
    // voltage to sag and the auth to time out (WIFI_REASON_AUTH_EXPIRE / 2).
    // Capping TX power to 8.5 dBm keeps current within regulator limits.
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    Serial.print(F("[wifi] connecting to " WIFI_SSID " "));
    const uint32_t deadline = millis() + 30000U;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        // Retry once with a clean reconnect
        Serial.print(F("[wifi] retry... "));
        WiFi.disconnect();
        delay(500);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        WiFi.setTxPower(WIFI_POWER_8_5dBm);
        const uint32_t deadline2 = millis() + 20000U;
        while (WiFi.status() != WL_CONNECTED && millis() < deadline2) {
            delay(500);
            Serial.print('.');
        }
        Serial.println();
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.print(F("[wifi] failed status="));
        Serial.print(WiFi.status());
        Serial.println(F(" — logs via USB only"));
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
