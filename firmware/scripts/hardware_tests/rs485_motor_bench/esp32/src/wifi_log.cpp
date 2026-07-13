#include "wifi_log.h"
#include "wifi_config.h"
#include "rs485_bench_config.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace {

static WiFiUDP g_udp;
static uint8_t g_node_id = 0;
static bool    g_udp_ready = false;

// Batch buffer: pack multiple log lines into one UDP datagram.
constexpr size_t kBatchCap = 480;
static char     g_batch[kBatchCap];
static size_t   g_batch_len = 0;
static uint32_t g_batch_started_ms = 0;
constexpr uint32_t kBatchFlushMs = 25U;  // flush coalesced traffic often

// Coalesce repeated high-rate RX cmd types (MOVE / HB) into one summary line.
static char     g_coal_cmd[12];   // "MOVE" / "HB" / ""
static char     g_coal_last[96];  // last full printable RX frame string
static uint32_t g_coal_count = 0;
static uint32_t g_coal_ok = 0;
static uint32_t g_coal_bad = 0;
static uint32_t g_coal_ignored = 0;
static uint32_t g_coal_started_ms = 0;

static void UdpWriteRaw(const char* line, size_t len) {
    if (!g_udp_ready || line == nullptr || len == 0) return;
    g_udp.beginPacket(LOG_HOST_IP, LOG_UDP_PORT);
    g_udp.write(reinterpret_cast<const uint8_t*>(line), len);
    g_udp.endPacket();
}

static void FlushBatch() {
    if (g_batch_len == 0) return;
    UdpWriteRaw(g_batch, g_batch_len);
    g_batch_len = 0;
    g_batch_started_ms = 0;
}

static void AppendBatchedLine(const char* line) {
    const size_t len = strlen(line);
    if (len == 0) return;

    // Need room for optional '\n' + line (+ NUL not stored).
    const size_t need = len + (g_batch_len > 0 ? 1U : 0U);
    if (need > kBatchCap) {
        // Oversized single line: flush batch, then send alone.
        FlushBatch();
        UdpWriteRaw(line, len);
        return;
    }
    if (g_batch_len > 0 && g_batch_len + need > kBatchCap) {
        FlushBatch();
    }
    if (g_batch_len == 0) {
        g_batch_started_ms = millis();
    } else {
        g_batch[g_batch_len++] = '\n';
    }
    memcpy(&g_batch[g_batch_len], line, len);
    g_batch_len += len;
}

static void FlushCoalesce() {
    if (g_coal_count == 0) return;

    char line[180];
    if (g_coal_count == 1) {
        snprintf(line, sizeof(line), "J%u|RX|%s", g_node_id, g_coal_last);
    } else {
        snprintf(line, sizeof(line),
                 "J%u|RX|%sx%lu last=%s ok=%lu bad=%lu ignored=%lu",
                 g_node_id,
                 g_coal_cmd,
                 static_cast<unsigned long>(g_coal_count),
                 g_coal_last,
                 static_cast<unsigned long>(g_coal_ok),
                 static_cast<unsigned long>(g_coal_bad),
                 static_cast<unsigned long>(g_coal_ignored));
    }
    AppendBatchedLine(line);
    g_coal_count = 0;
    g_coal_cmd[0] = '\0';
    g_coal_last[0] = '\0';
    g_coal_started_ms = 0;
}

static bool ExtractCoalKey(const char* msg, char* out_key, size_t key_cap) {
    // msg looks like: "@3 MOVE 45.00 30.00 ok=7 bad=0 ignored=2"
    //              or: "@* HB ok=..."
    if (msg == nullptr || out_key == nullptr || key_cap < 3) return false;
    const char* p = strchr(msg, ' ');
    if (p == nullptr) return false;
    ++p;
    while (*p == ' ') ++p;

    if (strncmp(p, "MOVE", 4) == 0 && (p[4] == ' ' || p[4] == '\0')) {
        strncpy(out_key, "MOVE", key_cap);
        out_key[key_cap - 1] = '\0';
        return true;
    }
    if (strncmp(p, "HB", 2) == 0 && (p[2] == ' ' || p[2] == '\0')) {
        strncpy(out_key, "HB", key_cap);
        out_key[key_cap - 1] = '\0';
        return true;
    }
    return false;
}

static void ParseCounters(const char* msg, uint32_t* ok, uint32_t* bad, uint32_t* ignored) {
    *ok = *bad = *ignored = 0;
    if (msg == nullptr) return;
    const char* p = strstr(msg, "ok=");
    if (p) *ok = static_cast<uint32_t>(atol(p + 3));
    p = strstr(msg, "bad=");
    if (p) *bad = static_cast<uint32_t>(atol(p + 4));
    p = strstr(msg, "ignored=");
    if (p) *ignored = static_cast<uint32_t>(atol(p + 8));
}

static void SendImmediate(const char* line) {
    FlushCoalesce();
    AppendBatchedLine(line);
    FlushBatch();  // important events don't wait
}

}  // namespace

bool WifiLogBegin(uint8_t node_id) {
    g_node_id = node_id;
    g_udp_ready = false;
    g_batch_len = 0;
    g_coal_count = 0;
    g_coal_cmd[0] = '\0';

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
    SendImmediate(boot_msg);

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

    // Always print full line to USB serial.
    Serial.print(F("[J"));
    Serial.print(g_node_id);
    Serial.print('|');
    Serial.print(tag);
    Serial.print(F("] "));
    Serial.println(msg);

    if (!g_udp_ready) return;

    // High-rate RX MOVE/HB: coalesce into compact summaries (no silent drops).
    if (tag != nullptr && strcmp(tag, "RX") == 0) {
        char key[12];
        if (ExtractCoalKey(msg, key, sizeof(key))) {
            // Extract frame text before " ok=" for last=
            char frame_only[96];
            strncpy(frame_only, msg, sizeof(frame_only) - 1);
            frame_only[sizeof(frame_only) - 1] = '\0';
            char* cut = strstr(frame_only, " ok=");
            if (cut) *cut = '\0';

            uint32_t ok = 0, bad = 0, ignored = 0;
            ParseCounters(msg, &ok, &bad, &ignored);

            if (g_coal_count > 0 && strcmp(g_coal_cmd, key) != 0) {
                FlushCoalesce();
                FlushBatch();
            }
            if (g_coal_count == 0) {
                strncpy(g_coal_cmd, key, sizeof(g_coal_cmd) - 1);
                g_coal_cmd[sizeof(g_coal_cmd) - 1] = '\0';
                g_coal_started_ms = millis();
            }
            strncpy(g_coal_last, frame_only, sizeof(g_coal_last) - 1);
            g_coal_last[sizeof(g_coal_last) - 1] = '\0';
            ++g_coal_count;
            g_coal_ok = ok;
            g_coal_bad = bad;
            g_coal_ignored = ignored;

            // Flush every ~25 ms or after 16 same-type frames.
            const uint32_t now = millis();
            if (g_coal_count >= 16U ||
                (now - g_coal_started_ms) >= kBatchFlushMs) {
                FlushCoalesce();
                FlushBatch();
            }
            return;
        }
    }

    // Everything else (BOOT/BAD/ENABLE/HOME/PING/…): send now, no drop.
    char line[160];
    snprintf(line, sizeof(line), "J%u|%s|%s", g_node_id, tag, msg);
    SendImmediate(line);
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

    if (!g_udp_ready) return;

    const uint32_t now = millis();
    if (g_coal_count > 0 && (now - g_coal_started_ms) >= kBatchFlushMs) {
        FlushCoalesce();
    }
    if (g_batch_len > 0 && (now - g_batch_started_ms) >= kBatchFlushMs) {
        FlushBatch();
    }
}
