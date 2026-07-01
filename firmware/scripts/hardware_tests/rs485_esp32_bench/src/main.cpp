// RS-485 multi-node bench slave — ESP32-C3.
//
// Listens on Serial1 (UART1) for addressed "@N PING\r\n" frames.
// Only handles frames where dst == this node's ID or dst == '*'.
// On match: sends ACK back on bus, WiFi-logs the event.
// Non-matching frames increment the 'ignored' counter so you can verify
// other nodes stayed quiet.
//
// Node ID priority:
//   1. Compile-time ROBOT_ARM_NODE_ID build flag (set by PlatformIO env)
//   2. NVS key "node_id" (set via "NODE_ID <n>" serial command)
//
// USB serial commands (115200):
//   NODE_ID <1-5>   save new ID to NVS and reboot
//   STATUS          print counters and WiFi state
//   help            print command list

#include <Arduino.h>
#include <Preferences.h>
#include "pinout.h"
#include "wifi_log.h"
#include "rs485_bench_config.h"
#include "rs485_bench_protocol.h"

using namespace esp32c3_slave;
using namespace rs485_bench;

// ── Node ID resolution ────────────────────────────────────────────────────────

static uint8_t ResolveNodeId() {
#if defined(ROBOT_ARM_NODE_ID) && ROBOT_ARM_NODE_ID >= 1 && ROBOT_ARM_NODE_ID <= 5
    return static_cast<uint8_t>(ROBOT_ARM_NODE_ID);
#else
    Preferences prefs;
    if (prefs.begin("bench_node", true)) {
        const uint32_t stored = prefs.getUInt("node_id", 0U);
        prefs.end();
        if (stored >= 1U && stored <= 5U) {
            return static_cast<uint8_t>(stored);
        }
    }
    return 0U;  // unconfigured — use NODE_ID command to set
#endif
}

// ── Globals ──────────────────────────────────────────────────────────────────

static uint8_t  g_node_id   = 0;
static uint32_t g_ok        = 0;
static uint32_t g_bad       = 0;
static uint32_t g_ignored   = 0;

// ── RS-485 helpers (Serial1 on ESP32-C3) ────────────────────────────────────

static void SetTransmitMode(bool tx) {
    digitalWrite(kRs485DePin, tx ? HIGH : LOW);
    if (tx) {
        delayMicroseconds(RS485_BENCH_DE_SETTLE_US);
    } else {
        delayMicroseconds(RS485_BENCH_DE_SETTLE_US);
    }
}

static void DrainRx() {
    const uint32_t deadline = millis() + RS485_BENCH_FRAME_GAP_MS + 2U;
    while (millis() < deadline) {
        while (Serial1.available() > 0) Serial1.read();
        delay(1);
    }
}

static size_t ReadFrame(uint8_t* out, size_t capacity) {
    if (Serial1.available() == 0) return 0U;

    size_t count = 0;
    uint32_t last_byte_ms = millis();

    while (count < capacity) {
        if (Serial1.available() > 0) {
            out[count++] = static_cast<uint8_t>(Serial1.read());
            last_byte_ms = millis();
            // End of frame: got CR+LF or LF
            if (count >= 2 &&
                out[count - 2] == '\r' && out[count - 1] == '\n') {
                break;
            }
        } else if (count > 0 &&
                   (millis() - last_byte_ms) >= RS485_BENCH_FRAME_GAP_MS) {
            break;
        }
    }
    return count;
}

static void SendAck() {
    char frame[RS485_BENCH_FRAME_MAX_LEN];
    const size_t len = BuildAckPingFrame(frame, sizeof(frame), g_node_id);
    if (len == 0) return;

    // Brief inter-frame gap before replying so bus is quiet
    delay(RS485_BENCH_FRAME_GAP_MS + 5U);

    DrainRx();
    SetTransmitMode(true);
    Serial1.write(reinterpret_cast<const uint8_t*>(frame), len);
    Serial1.flush();
    SetTransmitMode(false);
}

// ── USB command handler ────────────────────────────────────────────────────

static void HandleUsbCommand(const char* cmd) {
    while (*cmd == ' ') ++cmd;

    if (strncmp(cmd, "NODE_ID ", 8) == 0) {
        const int id = atoi(cmd + 8);
        if (id >= 1 && id <= 5) {
            Preferences prefs;
            prefs.begin("bench_node", false);
            prefs.putUInt("node_id", static_cast<uint32_t>(id));
            prefs.end();
            Serial.print(F("Node ID saved: "));
            Serial.print(id);
            Serial.println(F(" — rebooting..."));
            delay(500);
            ESP.restart();
        } else {
            Serial.println(F("usage: NODE_ID <1-5>"));
        }
        return;
    }
    if (strcmp(cmd, "STATUS") == 0 || strcmp(cmd, "status") == 0) {
        Serial.print(F("node_id="));
        Serial.print(g_node_id);
        Serial.print(F(" ok="));
        Serial.print(g_ok);
        Serial.print(F(" bad="));
        Serial.print(g_bad);
        Serial.print(F(" ignored="));
        Serial.print(g_ignored);
        Serial.print(F(" wifi="));
        Serial.println(WifiLogConnected() ? "up" : "down");
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        Serial.println(F("Commands: NODE_ID <1-5>  STATUS  help"));
        return;
    }
    Serial.print(F("unknown: "));
    Serial.println(cmd);
}

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup() {
    // USB CDC (Serial) for local debug
    Serial.begin(115200);
    // On ESP32-C3 native USB-CDC, wait for the host to open the port.
    // A brief extra settle delay prevents the boot banner from printing
    // before the USB connection is fully established in the host OS.
    for (uint32_t t = millis(); !Serial && millis() - t < 5000U;) {
        delay(10);
    }
    delay(200);  // settle: USB enumeration can race with first Serial.print

    g_node_id = ResolveNodeId();

    // RS-485 on Serial1 (UART1)
    pinMode(kRs485DePin, OUTPUT);
    SetTransmitMode(false);
    Serial1.begin(RS485_BENCH_BAUD, SERIAL_8N1, kRs485RxPin, kRs485TxPin);
    DrainRx();

    pinMode(kStatusLedPin, OUTPUT);
    digitalWrite(kStatusLedPin, LOW);

    Serial.println();
    Serial.println(F("========================================="));
    Serial.print(F(" RS-485 bench slave — node id="));
    Serial.println(g_node_id == 0 ? '?' : static_cast<char>('0' + g_node_id));
    Serial.println(F("========================================="));
    if (g_node_id == 0) {
        Serial.println(F("  WARNING: node ID not set! Use: NODE_ID <1-5>"));
    }
    Serial.print(F("  Bus baud : "));
    Serial.println(RS485_BENCH_BAUD);
    Serial.println(F("  UART1: GPIO4=TX, GPIO5=RX, GPIO6=DE"));
    Serial.println(F("  Type 'help' for commands."));
    Serial.println();

    WifiLogBegin(g_node_id);
}

void loop() {
    // ── WiFi maintenance ────────────────────────────────────────────────────
    WifiLogPoll();

    // ── USB serial commands ─────────────────────────────────────────────────
    static char usb_buf[64];
    static uint8_t usb_len = 0;

    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\r' || c == '\n') {
            if (usb_len > 0) {
                usb_buf[usb_len] = '\0';
                HandleUsbCommand(usb_buf);
                usb_len = 0;
            }
        } else if (usb_len < sizeof(usb_buf) - 1U) {
            usb_buf[usb_len++] = c;
        }
    }

    // ── RS-485 receive loop ─────────────────────────────────────────────────
    if (g_node_id == 0) return;  // refuse to act without a valid ID

    uint8_t rx_buf[RS485_BENCH_FRAME_MAX_LEN + 4U];
    const size_t rx_len = ReadFrame(rx_buf, sizeof(rx_buf));
    if (rx_len == 0) return;

    const bool plausible = IsPlausibleFrame(rx_buf, rx_len);
    if (!plausible) {
        ++g_bad;
        // Build hex string for logging
        char hex[RS485_BENCH_FRAME_MAX_LEN * 3 + 4];
        size_t hpos = 0;
        for (size_t i = 0; i < rx_len && hpos + 3 < sizeof(hex); ++i) {
            snprintf(&hex[hpos], 4, "%02X ", rx_buf[i]);
            hpos += 3;
        }
        if (hpos > 0) hex[hpos - 1] = '\0';
        WifiLog("BAD", "len=%u hex=%s bad=%lu", rx_len, hex, (unsigned long)g_bad);
        return;
    }

    const ParsedFrame pf = ParseFrame(rx_buf, rx_len);
    if (!pf.valid) {
        ++g_bad;
        return;
    }

    if (!FrameMatchesNode(pf, g_node_id)) {
        ++g_ignored;
        return;  // addressed to another node — stay silent
    }

    // This frame is for us
    ++g_ok;

    // Build printable copy of the received frame (strip CRLF)
    char frame_str[RS485_BENCH_FRAME_MAX_LEN + 2];
    size_t slen = 0;
    for (size_t i = 0; i < rx_len && slen < sizeof(frame_str) - 1U; ++i) {
        if (rx_buf[i] != '\r' && rx_buf[i] != '\n') {
            frame_str[slen++] = static_cast<char>(rx_buf[i]);
        }
    }
    frame_str[slen] = '\0';

    WifiLog("RX", "%s ok=%lu bad=%lu ignored=%lu",
            frame_str,
            (unsigned long)g_ok,
            (unsigned long)g_bad,
            (unsigned long)g_ignored);

    // Blink LED
    digitalWrite(kStatusLedPin, HIGH);
    SendAck();
    digitalWrite(kStatusLedPin, LOW);
}
