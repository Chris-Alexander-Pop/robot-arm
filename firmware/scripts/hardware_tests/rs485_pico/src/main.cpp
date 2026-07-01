// RS-485 multi-node bench master — Raspberry Pi Pico.
//
// USB serial commands (115200):
//   ping <id>      send @<id> PING to one node   (id = 1..5)
//   ping all       broadcast @* PING to all nodes
//   <id>           shorthand for ping <id>
//   all            shorthand for ping all
//   auto [ms]      start auto-sweep (default 2000 ms between rounds)
//   stop           stop auto-sweep
//   status         print counters
//   help           print this list
//
// The bus replies (@N ACK PING\r\n) are printed on the USB monitor so you
// can verify addressing without needing WiFi.

#include <Arduino.h>
#include "pinout.h"
#include "rs485_bench_config.h"
#include "rs485_bench_protocol.h"

using namespace pico_master;
using namespace rs485_bench;

// ── Globals ──────────────────────────────────────────────────────────────────

static UART Serial1Obj(kRs485UartTx, kRs485UartRx, -1, -1);

static uint32_t g_pings_sent   = 0;
static uint32_t g_replies_ok   = 0;
static uint32_t g_replies_bad  = 0;

// Auto-sweep state
static bool     g_auto_active   = false;
static uint32_t g_auto_interval = 2000U;   // ms between rounds
static uint32_t g_auto_last_ms  = 0U;
static uint8_t  g_auto_next_id  = 1U;

// ── RS-485 helpers ────────────────────────────────────────────────────────────

static void SetTransmitMode(bool tx) {
    digitalWrite(kRs485DePin, tx ? HIGH : LOW);
    if (tx) {
        delayMicroseconds(RS485_BENCH_DE_SETTLE_US);
    } else {
        delayMicroseconds(RS485_BENCH_DE_SETTLE_US);
    }
}

static void DrainRx() {
    const uint32_t deadline = millis() + RS485_BENCH_FRAME_GAP_MS;
    while (millis() < deadline) {
        while (Serial1Obj.available() > 0) {
            Serial1Obj.read();
        }
        delay(1);
    }
}

static size_t ReadFrame(uint8_t* out, size_t capacity, uint32_t timeout_ms) {
    const uint32_t deadline = millis() + timeout_ms;
    size_t count = 0;
    uint32_t last_byte_ms = 0;

    while (millis() < deadline && count < capacity) {
        if (Serial1Obj.available() > 0) {
            out[count++] = static_cast<uint8_t>(Serial1Obj.read());
            last_byte_ms = millis();
            // End of frame: CRLF received
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

// ── Send a single addressed ping ─────────────────────────────────────────────

static void SendPing(char dst) {
    char frame[RS485_BENCH_FRAME_MAX_LEN];
    const size_t len = BuildPingFrame(frame, sizeof(frame), dst);
    if (len == 0) {
        return;
    }

    DrainRx();
    SetTransmitMode(true);
    Serial1Obj.write(reinterpret_cast<const uint8_t*>(frame), len);
    Serial1Obj.flush();
    SetTransmitMode(false);

    ++g_pings_sent;
    digitalWrite(kLedPin, HIGH);

    // Print what we sent
    Serial.print(F("[TX] "));
    Serial.print(frame);  // already has \r\n; print() handles it fine

    // Listen for reply (~80 ms, same as STM32 bench)
    uint8_t rx_buf[RS485_BENCH_FRAME_MAX_LEN + 4U];
    const size_t rx_len = ReadFrame(rx_buf, sizeof(rx_buf), 80U);

    digitalWrite(kLedPin, LOW);

    if (rx_len == 0) {
        Serial.println(F("     (no reply)"));
        return;
    }

    const bool plausible = IsPlausibleFrame(rx_buf, rx_len);
    const ParsedFrame pf = ParseFrame(rx_buf, rx_len);

    if (plausible && pf.valid && pf.is_ack) {
        ++g_replies_ok;
    } else {
        ++g_replies_bad;
    }

    Serial.print(plausible ? F("[RX OK ] ") : F("[RX BAD] "));
    for (size_t i = 0; i < rx_len; ++i) {
        const char c = static_cast<char>(rx_buf[i]);
        if (c == '\r' || c == '\n') continue;
        Serial.print(c);
    }
    Serial.print(F("  (ok="));
    Serial.print(g_replies_ok);
    Serial.print(F(" bad="));
    Serial.print(g_replies_bad);
    Serial.println(')');
}

// Broadcast and collect up to 5 replies (one per node, spaced ~20 ms apart)
static void SendBroadcastPing() {
    char frame[RS485_BENCH_FRAME_MAX_LEN];
    const size_t len = BuildPingFrame(frame, sizeof(frame), RS485_BENCH_ADDR_BROADCAST);
    if (len == 0) return;

    DrainRx();
    SetTransmitMode(true);
    Serial1Obj.write(reinterpret_cast<const uint8_t*>(frame), len);
    Serial1Obj.flush();
    SetTransmitMode(false);

    ++g_pings_sent;
    digitalWrite(kLedPin, HIGH);
    Serial.print(F("[TX] "));
    Serial.print(frame);

    // Collect replies for 200 ms
    const uint32_t collect_deadline = millis() + 200U;
    uint8_t reply_count = 0;
    while (millis() < collect_deadline) {
        uint8_t rx_buf[RS485_BENCH_FRAME_MAX_LEN + 4U];
        const size_t rx_len = ReadFrame(rx_buf, sizeof(rx_buf), 30U);
        if (rx_len == 0) break;

        const bool plausible = IsPlausibleFrame(rx_buf, rx_len);
        const ParsedFrame pf = ParseFrame(rx_buf, rx_len);

        if (plausible && pf.valid && pf.is_ack) {
            ++g_replies_ok;
        } else if (rx_len > 0) {
            ++g_replies_bad;
        }

        Serial.print(plausible ? F("[RX OK ] ") : F("[RX BAD] "));
        for (size_t i = 0; i < rx_len; ++i) {
            const char c = static_cast<char>(rx_buf[i]);
            if (c == '\r' || c == '\n') continue;
            Serial.print(c);
        }
        Serial.println();
        ++reply_count;
        if (reply_count >= 5) break;
    }
    digitalWrite(kLedPin, LOW);
    Serial.print(F("     -> "));
    Serial.print(reply_count);
    Serial.println(F(" replies collected"));
}

// ── USB command parser ────────────────────────────────────────────────────────

static void PrintHelp() {
    Serial.println(F("Commands:"));
    Serial.println(F("  ping <id>  — ping one node (1-5)"));
    Serial.println(F("  ping all   — broadcast ping"));
    Serial.println(F("  <id>       — shorthand for ping <id>"));
    Serial.println(F("  all        — shorthand for ping all"));
    Serial.println(F("  auto [ms]  — start auto-sweep (default 2000 ms)"));
    Serial.println(F("  stop       — stop auto-sweep"));
    Serial.println(F("  status     — print counters"));
    Serial.println(F("  help       — this message"));
}

static void PrintStatus() {
    Serial.print(F("pings_sent="));
    Serial.print(g_pings_sent);
    Serial.print(F(" replies_ok="));
    Serial.print(g_replies_ok);
    Serial.print(F(" replies_bad="));
    Serial.print(g_replies_bad);
    Serial.print(F(" auto="));
    Serial.print(g_auto_active ? "on" : "off");
    if (g_auto_active) {
        Serial.print(F(" interval_ms="));
        Serial.print(g_auto_interval);
    }
    Serial.println();
}

static void HandleCommand(const char* cmd) {
    // Trim leading space
    while (*cmd == ' ') ++cmd;

    // Shorthand: bare digit
    if (cmd[0] >= '1' && cmd[0] <= '5' && cmd[1] == '\0') {
        SendPing(cmd[0]);
        return;
    }
    if (strcmp(cmd, "all") == 0) {
        SendBroadcastPing();
        return;
    }
    if (strncmp(cmd, "ping ", 5) == 0) {
        const char* arg = cmd + 5;
        while (*arg == ' ') ++arg;
        if (strcmp(arg, "all") == 0) {
            SendBroadcastPing();
        } else if (arg[0] >= '1' && arg[0] <= '5') {
            SendPing(arg[0]);
        } else {
            Serial.println(F("usage: ping <1-5|all>"));
        }
        return;
    }
    if (strncmp(cmd, "auto", 4) == 0) {
        const char* arg = cmd + 4;
        while (*arg == ' ') ++arg;
        if (*arg != '\0') {
            const uint32_t ms = static_cast<uint32_t>(atol(arg));
            if (ms >= 100U) {
                g_auto_interval = ms;
            }
        }
        g_auto_active = true;
        g_auto_last_ms = millis();
        g_auto_next_id = 1U;
        Serial.print(F("Auto-sweep started, interval="));
        Serial.print(g_auto_interval);
        Serial.println(F(" ms"));
        return;
    }
    if (strcmp(cmd, "stop") == 0) {
        g_auto_active = false;
        Serial.println(F("Auto-sweep stopped."));
        return;
    }
    if (strcmp(cmd, "status") == 0) {
        PrintStatus();
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        PrintHelp();
        return;
    }
    Serial.print(F("unknown command: "));
    Serial.println(cmd);
    Serial.println(F("Type 'help' for commands."));
}

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    // Wait up to 3 s for host to open serial (optional on Pico USB)
    for (uint32_t t = millis(); !Serial && millis() - t < 3000U;) {
        delay(10);
    }

    pinMode(kRs485DePin, OUTPUT);
    SetTransmitMode(false);

    pinMode(kLedPin, OUTPUT);
    digitalWrite(kLedPin, LOW);

    Serial1Obj.begin(RS485_BENCH_BAUD);
    DrainRx();

    Serial.println();
    Serial.println(F("========================================="));
    Serial.println(F(" RS-485 multi-node bench — Pico master"));
    Serial.println(F("========================================="));
    Serial.print(F("  Bus baud : "));
    Serial.println(RS485_BENCH_BAUD);
    Serial.println(F("  UART0: GPIO0=TX, GPIO1=RX, GPIO2=DE"));
    Serial.println(F("  Type 'help' for commands."));
    Serial.println();
}

void loop() {
    // ── Handle USB serial input ─────────────────────────────────────────────
    static char cmd_buf[64];
    static uint8_t cmd_len = 0;

    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\r' || c == '\n') {
            Serial.print(F("\r\n"));
            if (cmd_len > 0) {
                cmd_buf[cmd_len] = '\0';
                HandleCommand(cmd_buf);
                cmd_len = 0;
            }
        } else if (c == '\b' || c == 0x7F) {
            // Backspace / DEL — erase last char
            if (cmd_len > 0) {
                --cmd_len;
                Serial.print(F("\b \b"));  // move back, overwrite with space, move back
            }
        } else if (cmd_len < sizeof(cmd_buf) - 1U) {
            cmd_buf[cmd_len++] = c;
            Serial.print(c);  // echo the character so you see what you're typing
        }
    }

    // ── Auto-sweep ──────────────────────────────────────────────────────────
    if (g_auto_active) {
        const uint32_t now = millis();
        if (now - g_auto_last_ms >= g_auto_interval) {
            g_auto_last_ms = now;
            SendPing(static_cast<char>('0' + g_auto_next_id));
            ++g_auto_next_id;
            if (g_auto_next_id > 5U) {
                g_auto_next_id = 1U;
            }
        }
    }
}
