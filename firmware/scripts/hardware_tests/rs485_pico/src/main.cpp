// RS-485 multi-node bench master — Raspberry Pi Pico.
//
// USB serial commands (115200):
//   ping <id>      send @<id> PING to one node   (id = 1..5)
//   ping all       broadcast @* PING to all nodes
//   <id>           shorthand for ping <id>
//   all            shorthand for ping all
//   move <id> <pos> <vel>   dummy SET_JOINT_TARGET  (@id MOVE pos vel)
//   enable <id> <0|1>       dummy ENABLE
//   home <id>               dummy HOME
//   hb                      broadcast HEARTBEAT (@* HB)
//   stress [rounds] [ms]    high-rate MOVE round-robin (default 100 r @ 20 ms)
//   auto [ms]      start auto-sweep (default 2000 ms between rounds)
//   stop           stop auto-sweep / stress
//   status         print counters
//   help           print this list
//
// One-way bench: master TX-only; slaves log over WiFi (no bus ACKs).

#include <Arduino.h>
#include <math.h>
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

// Stress: round-robin dummy joint targets (ASCII stand-in for SET_JOINT_TARGET)
static bool     g_stress_active   = false;
static uint32_t g_stress_rounds   = 0U;     // remaining full 1..5 rounds
static uint32_t g_stress_period   = 20U;    // ms between individual frames
static uint32_t g_stress_last_ms  = 0U;
static uint8_t  g_stress_next_id  = 1U;
static uint32_t g_stress_seq      = 0U;
static uint32_t g_moves_sent      = 0U;
static uint32_t g_hb_sent         = 0U;

// ── RS-485 helpers ────────────────────────────────────────────────────────────

static void SetTransmitMode(bool tx) {
    if (kRs485TxOnly) {
        // One-way bench: DE stays HIGH forever.
        digitalWrite(kRs485DePin, HIGH);
        delayMicroseconds(RS485_BENCH_DE_SETTLE_US);
        return;
    }
    digitalWrite(kRs485DePin, tx ? HIGH : LOW);
    delayMicroseconds(RS485_BENCH_DE_SETTLE_US);
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

// ── Generic TX helper ────────────────────────────────────────────────────────

static bool BusSend(char dst, const char* cmd, bool quiet) {
    char frame[RS485_BENCH_FRAME_MAX_LEN];
    const size_t len = BuildFrame(frame, sizeof(frame), dst, cmd);
    if (len == 0) {
        return false;
    }

    if (!kRs485TxOnly) {
        DrainRx();
    }
    SetTransmitMode(true);
    Serial1Obj.write(reinterpret_cast<const uint8_t*>(frame), len);
    Serial1Obj.flush();
    if (!kRs485TxOnly) {
        SetTransmitMode(false);
    }

    digitalWrite(kLedPin, HIGH);
    if (!quiet) {
        Serial.print(F("[TX] "));
        Serial.print(frame);
        if (kRs485TxOnly) {
            Serial.println(F("     (TX-only — watch ESP WiFi/USB logs)"));
        }
    }
    digitalWrite(kLedPin, LOW);
    return true;
}

// ── Send a single addressed ping ─────────────────────────────────────────────

static void SendPing(char dst) {
    char frame[RS485_BENCH_FRAME_MAX_LEN];
    const size_t len = BuildPingFrame(frame, sizeof(frame), dst);
    if (len == 0) {
        return;
    }

    if (!kRs485TxOnly) {
        DrainRx();
    }
    SetTransmitMode(true);
    Serial1Obj.write(reinterpret_cast<const uint8_t*>(frame), len);
    Serial1Obj.flush();
    if (!kRs485TxOnly) {
        SetTransmitMode(false);
    }

    ++g_pings_sent;
    digitalWrite(kLedPin, HIGH);

    // Print what we sent
    Serial.print(F("[TX] "));
    Serial.print(frame);  // already has \r\n; print() handles it fine

    if (kRs485TxOnly) {
        digitalWrite(kLedPin, LOW);
        Serial.println(F("     (TX-only — watch ESP WiFi/USB logs)"));
        return;
    }

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

    if (!kRs485TxOnly) {
        DrainRx();
    }
    SetTransmitMode(true);
    Serial1Obj.write(reinterpret_cast<const uint8_t*>(frame), len);
    Serial1Obj.flush();
    if (!kRs485TxOnly) {
        SetTransmitMode(false);
    }

    ++g_pings_sent;
    digitalWrite(kLedPin, HIGH);
    Serial.print(F("[TX] "));
    Serial.print(frame);

    if (kRs485TxOnly) {
        digitalWrite(kLedPin, LOW);
        Serial.println(F("     (TX-only broadcast — watch ESP WiFi/USB logs)"));
        return;
    }

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
    Serial.println(F("  move <id> <pos> <vel> — dummy SET_JOINT_TARGET"));
    Serial.println(F("  enable <id> <0|1>     — dummy ENABLE"));
    Serial.println(F("  home <id>             — dummy HOME"));
    Serial.println(F("  hb                    — broadcast HEARTBEAT"));
    Serial.println(F("  stress [rounds] [ms]  — MOVE sweep (def 100r @ 20ms)"));
    Serial.println(F("  auto [ms]  — start auto-sweep (default 2000 ms)"));
    Serial.println(F("  stop       — stop auto-sweep / stress"));
    Serial.println(F("  status     — print counters"));
    Serial.println(F("  help       — this message"));
}

static void PrintStatus() {
    Serial.print(F("pings_sent="));
    Serial.print(g_pings_sent);
    Serial.print(F(" moves_sent="));
    Serial.print(g_moves_sent);
    Serial.print(F(" hb_sent="));
    Serial.print(g_hb_sent);
    Serial.print(F(" replies_ok="));
    Serial.print(g_replies_ok);
    Serial.print(F(" replies_bad="));
    Serial.print(g_replies_bad);
    Serial.print(F(" auto="));
    Serial.print(g_auto_active ? "on" : "off");
    Serial.print(F(" stress="));
    Serial.print(g_stress_active ? "on" : "off");
    if (g_auto_active) {
        Serial.print(F(" interval_ms="));
        Serial.print(g_auto_interval);
    }
    if (g_stress_active) {
        Serial.print(F(" rounds_left="));
        Serial.print(g_stress_rounds);
        Serial.print(F(" period_ms="));
        Serial.print(g_stress_period);
    }
    Serial.println();
}

static void SendMove(uint8_t node_id, float pos_deg, float vel_dps, bool quiet) {
    char cmd[36];
    // Keep ASCII short enough for RS485_BENCH_FRAME_MAX_LEN (48).
    snprintf(cmd, sizeof(cmd), "MOVE %.2f %.2f", static_cast<double>(pos_deg),
             static_cast<double>(vel_dps));
    if (BusSend(static_cast<char>('0' + node_id), cmd, quiet)) {
        ++g_moves_sent;
    }
}

static void SendEnable(uint8_t node_id, uint8_t on) {
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "ENABLE %u", static_cast<unsigned>(on ? 1U : 0U));
    BusSend(static_cast<char>('0' + node_id), cmd, false);
}

static void SendHome(uint8_t node_id) {
    BusSend(static_cast<char>('0' + node_id), "HOME", false);
}

static void SendHeartbeat() {
    if (BusSend(RS485_BENCH_ADDR_BROADCAST, "HB", false)) {
        ++g_hb_sent;
    }
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
    if (strncmp(cmd, "move ", 5) == 0) {
        const char* arg = cmd + 5;
        while (*arg == ' ') ++arg;
        if (arg[0] < '1' || arg[0] > '5') {
            Serial.println(F("usage: move <1-5> <pos_deg> <vel_dps>"));
            return;
        }
        const uint8_t id = static_cast<uint8_t>(arg[0] - '0');
        ++arg;
        while (*arg == ' ') ++arg;
        char* end = nullptr;
        const float pos = strtof(arg, &end);
        if (end == arg) {
            Serial.println(F("usage: move <1-5> <pos_deg> <vel_dps>"));
            return;
        }
        arg = end;
        while (*arg == ' ') ++arg;
        const float vel = strtof(arg, &end);
        if (end == arg) {
            Serial.println(F("usage: move <1-5> <pos_deg> <vel_dps>"));
            return;
        }
        SendMove(id, pos, vel, false);
        return;
    }
    if (strncmp(cmd, "enable ", 7) == 0) {
        const char* arg = cmd + 7;
        while (*arg == ' ') ++arg;
        if (arg[0] < '1' || arg[0] > '5') {
            Serial.println(F("usage: enable <1-5> <0|1>"));
            return;
        }
        const uint8_t id = static_cast<uint8_t>(arg[0] - '0');
        ++arg;
        while (*arg == ' ') ++arg;
        SendEnable(id, (*arg == '1') ? 1U : 0U);
        return;
    }
    if (strncmp(cmd, "home ", 5) == 0) {
        const char* arg = cmd + 5;
        while (*arg == ' ') ++arg;
        if (arg[0] < '1' || arg[0] > '5') {
            Serial.println(F("usage: home <1-5>"));
            return;
        }
        SendHome(static_cast<uint8_t>(arg[0] - '0'));
        return;
    }
    if (strcmp(cmd, "hb") == 0) {
        SendHeartbeat();
        return;
    }
    if (strncmp(cmd, "stress", 6) == 0) {
        const char* arg = cmd + 6;
        while (*arg == ' ') ++arg;
        uint32_t rounds = 100U;
        uint32_t period = 20U;
        if (*arg != '\0') {
            rounds = static_cast<uint32_t>(atol(arg));
            while (*arg != '\0' && *arg != ' ') ++arg;
            while (*arg == ' ') ++arg;
            if (*arg != '\0') {
                period = static_cast<uint32_t>(atol(arg));
            }
        }
        if (rounds < 1U) rounds = 1U;
        if (period < 5U) period = 5U;
        g_auto_active = false;
        g_stress_active = true;
        g_stress_rounds = rounds;
        g_stress_period = period;
        g_stress_last_ms = millis();
        g_stress_next_id = 1U;
        g_stress_seq = 0U;
        Serial.print(F("Stress started: "));
        Serial.print(rounds);
        Serial.print(F(" rounds x nodes 1-5, period="));
        Serial.print(period);
        Serial.println(F(" ms (dummy MOVE traffic)"));
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
        g_stress_active = false;
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
        g_stress_active = false;
        Serial.println(F("Auto-sweep / stress stopped."));
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
    SetTransmitMode(kRs485TxOnly);  // TX-only: leave DE HIGH

    pinMode(kLedPin, OUTPUT);
    digitalWrite(kLedPin, LOW);

    Serial1Obj.begin(RS485_BENCH_BAUD);
    if (!kRs485TxOnly) {
        DrainRx();
    }

    Serial.println();
    Serial.println(F("========================================="));
    Serial.println(F(" RS-485 multi-node bench — Pico master"));
    Serial.println(F("========================================="));
    Serial.print(F("  Bus baud : "));
    Serial.println(RS485_BENCH_BAUD);
    Serial.println(F("  UART0: GPIO0=TX, GPIO1=RX, GPIO2=DE"));
    if (kRs485TxOnly) {
        Serial.println(F("  Mode: TX-only (DE held HIGH, no bus RX)"));
    }
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

    // ── Stress: high-rate dummy joint targets ───────────────────────────────
    if (g_stress_active) {
        const uint32_t now = millis();
        if (now - g_stress_last_ms >= g_stress_period) {
            g_stress_last_ms = now;
            ++g_stress_seq;
            // Synthetic trajectory: +/- 60 deg at ~30 deg/s, phase-offset per joint
            const float phase = static_cast<float>(g_stress_seq + g_stress_next_id * 17U);
            const float pos = 60.0f * sinf(phase * 0.05f);
            const float vel = 30.0f * cosf(phase * 0.05f);
            const bool quiet = (g_stress_seq % 25U) != 0U;  // print ~every 25th TX
            SendMove(g_stress_next_id, pos, vel, quiet);

            // Heartbeat every full round of joints
            if (g_stress_next_id == 5U) {
                BusSend(RS485_BENCH_ADDR_BROADCAST, "HB", true);
                ++g_hb_sent;
                if (g_stress_rounds > 0U) {
                    --g_stress_rounds;
                }
                if (g_stress_rounds == 0U) {
                    g_stress_active = false;
                    Serial.println(F("Stress complete."));
                    PrintStatus();
                }
            }

            ++g_stress_next_id;
            if (g_stress_next_id > 5U) {
                g_stress_next_id = 1U;
            }
        }
    }
}
