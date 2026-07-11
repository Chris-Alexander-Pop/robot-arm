// RS-485 motor bench master — Raspberry Pi Pico.
//
// USB serial commands (115200):
//   run all <steps> <dir> <hz>   broadcast RUN to all nodes
//   run <id> <steps> <dir> <hz>  unicast RUN
//   stop / stop all              broadcast STOP
//   jog all <dir> <hz>           broadcast JOG
//   jog <id> <dir> <hz>          unicast JOG
//   bench                        scripted parallel motor test
//   wave [all|<id>] [steps] [hz] continuous fwd/rev (default all 1000 @ 500)
//   ping <id|all>                comms sanity check
//   status / help

#include <Arduino.h>
#include "pinout.h"
#include "rs485_bench_config.h"
#include "rs485_bench_protocol.h"
#include "rs485_motor_protocol.h"

using namespace pico_master;
using namespace rs485_bench;
using namespace rs485_motor;

static UART Serial1Obj(kRs485UartTx, kRs485UartRx, -1, -1);

static uint32_t g_frames_sent = 0;
static uint32_t g_bench_runs  = 0;

enum class BenchPhase : uint8_t {
    Idle,
    CreepFwd,
    Wait1,
    CreepRev,
    Wait2,
    RevFwd,
    Wait3,
    RevRev,
    Stop,
    Done,
};

static bool      g_bench_active = false;
static BenchPhase g_bench_phase = BenchPhase::Idle;
static uint32_t  g_bench_wait_until = 0U;

// Continuous back-and-forth: RUN steps dir=1, then RUN steps dir=0, repeat.
// dir: 1 = forward, 0 = reverse (not 2).
static bool     g_wave_active = false;
static char     g_wave_dst    = RS485_BENCH_ADDR_BROADCAST;
static uint8_t  g_wave_dir    = 1U;   // next direction to send
static uint32_t g_wave_steps  = 1000U;
static uint32_t g_wave_hz     = 500U;
static uint32_t g_wave_wait_until = 0U;
static uint32_t g_wave_cycles = 0U;

static uint32_t WaveDwellMs() {
    // Motion time + small gap so nodes finish before the reverse command.
    const uint32_t motion_ms =
        (g_wave_hz > 0U) ? ((g_wave_steps * 1000U) / g_wave_hz) : 2000U;
    return motion_ms + 400U;
}

static void SetTransmitMode(bool /*tx*/) {
    digitalWrite(kRs485DePin, HIGH);
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

static bool BusSendFrame(const char* frame, size_t len, bool quiet) {
    if (frame == nullptr || len == 0U) {
        return false;
    }

    if (!kRs485TxOnly) {
        DrainRx();
    }
    SetTransmitMode(true);
    Serial1Obj.write(reinterpret_cast<const uint8_t*>(frame), len);
    Serial1Obj.flush();

    ++g_frames_sent;
    digitalWrite(kLedPin, HIGH);
    if (!quiet) {
        Serial.print(F("[TX] "));
        Serial.print(frame);
        Serial.println(F("     (watch ESP WiFi logs)"));
    }
    digitalWrite(kLedPin, LOW);
    return true;
}

static bool BusSendRun(char dst, uint32_t steps, uint8_t dir, uint32_t hz, bool quiet) {
    char frame[RS485_BENCH_FRAME_MAX_LEN];
    const size_t len = BuildRunFrame(frame, sizeof(frame), dst, steps, dir, hz);
    if (len == 0U) {
        return false;
    }
    return BusSendFrame(frame, len, quiet);
}

static bool BusSendStop(char dst, bool quiet) {
    char frame[RS485_BENCH_FRAME_MAX_LEN];
    const size_t len = BuildStopFrame(frame, sizeof(frame), dst);
    if (len == 0U) {
        return false;
    }
    return BusSendFrame(frame, len, quiet);
}

static bool BusSendJog(char dst, uint8_t dir, uint32_t hz, bool quiet) {
    char frame[RS485_BENCH_FRAME_MAX_LEN];
    const size_t len = BuildJogFrame(frame, sizeof(frame), dst, dir, hz);
    if (len == 0U) {
        return false;
    }
    return BusSendFrame(frame, len, quiet);
}

static bool BusSendHold(char dst, uint8_t dir, bool quiet) {
    char frame[RS485_BENCH_FRAME_MAX_LEN];
    const size_t len = BuildHoldFrame(frame, sizeof(frame), dst, dir);
    if (len == 0U) {
        return false;
    }
    return BusSendFrame(frame, len, quiet);
}

static void SendPing(char dst) {
    char frame[RS485_BENCH_FRAME_MAX_LEN];
    const size_t len = BuildPingFrame(frame, sizeof(frame), dst);
    if (len > 0U) {
        BusSendFrame(frame, len, false);
    }
}

static void PrintHelp() {
    Serial.println(F("Commands:"));
    Serial.println(F("  run all <steps> <dir> <hz>  — broadcast RUN"));
    Serial.println(F("  run <id> <steps> <dir> <hz> — unicast RUN"));
    Serial.println(F("  stop / stop all             — broadcast STOP"));
    Serial.println(F("  jog all <dir> <hz>          — broadcast JOG"));
    Serial.println(F("  jog <id> <dir> <hz>         — unicast JOG"));
    Serial.println(F("  hold all <0|1>              — steady DIR (multimeter)"));
    Serial.println(F("  hold <id> <0|1>             — unicast HOLD"));
    Serial.println(F("  bench                       — scripted parallel test"));
    Serial.println(F("  wave [all|1-4] [steps] [hz] — back/forth (def all 1000 500)"));
    Serial.println(F("                              dir 1=fwd, 0=rev; stop to halt"));
    Serial.println(F("  ping <id|all>               — comms check"));
    Serial.println(F("  status / help"));
}

static void PrintStatus() {
    Serial.print(F("frames_sent="));
    Serial.print(g_frames_sent);
    Serial.print(F(" bench_runs="));
    Serial.print(g_bench_runs);
    Serial.print(F(" bench="));
    Serial.print(g_bench_active ? "on" : "off");
    Serial.print(F(" wave="));
    Serial.print(g_wave_active ? "on" : "off");
    if (g_wave_active) {
        Serial.print(F(" cycles="));
        Serial.print(g_wave_cycles);
        Serial.print(F(" dst="));
        Serial.print(g_wave_dst);
        Serial.print(F(" steps="));
        Serial.print(g_wave_steps);
        Serial.print(F(" hz="));
        Serial.print(g_wave_hz);
    }
    Serial.println();
}

static void StartBench() {
    g_wave_active = false;
    g_bench_active = true;
    g_bench_phase = BenchPhase::CreepFwd;
    g_bench_wait_until = 0U;
    ++g_bench_runs;
    Serial.println(F("Bench started: creep fwd -> creep rev -> 1 rev fwd/rev"));
}

static void StartWave(char dst, uint32_t steps, uint32_t hz) {
    if (steps < 1U) {
        steps = 1000U;
    }
    if (hz < 1U) {
        hz = 500U;
    }
    g_bench_active = false;
    g_bench_phase = BenchPhase::Idle;
    g_wave_active = true;
    g_wave_dst = dst;
    g_wave_steps = steps;
    g_wave_hz = hz;
    g_wave_dir = 1U;
    g_wave_wait_until = 0U;
    g_wave_cycles = 0U;
    Serial.print(F("Wave started: dst="));
    Serial.print(dst);
    Serial.print(F(" "));
    Serial.print(steps);
    Serial.print(F(" steps @ "));
    Serial.print(hz);
    Serial.println(F(" Hz  (dir 1 <-> 0). Type stop to halt."));
}

static void WaveTick() {
    if (!g_wave_active) {
        return;
    }

    const uint32_t now = millis();
    if (now < g_wave_wait_until) {
        return;
    }

    BusSendRun(g_wave_dst, g_wave_steps, g_wave_dir, g_wave_hz, false);
    if (g_wave_dir == 0U) {
        ++g_wave_cycles;
    }
    g_wave_dir = (g_wave_dir == 0U) ? 1U : 0U;
    g_wave_wait_until = now + WaveDwellMs();
}

static void BenchTick() {
    if (!g_bench_active) {
        return;
    }

    const uint32_t now = millis();

    switch (g_bench_phase) {
        case BenchPhase::Idle:
            return;

        case BenchPhase::CreepFwd:
            BusSendRun(RS485_BENCH_ADDR_BROADCAST, 200U, 1U, 50U, false);
            g_bench_phase = BenchPhase::Wait1;
            g_bench_wait_until = now + 2500U;
            break;

        case BenchPhase::Wait1:
            if (now < g_bench_wait_until) {
                return;
            }
            g_bench_phase = BenchPhase::CreepRev;
            break;

        case BenchPhase::CreepRev:
            BusSendRun(RS485_BENCH_ADDR_BROADCAST, 200U, 0U, 50U, false);
            g_bench_phase = BenchPhase::Wait2;
            g_bench_wait_until = now + 2500U;
            break;

        case BenchPhase::Wait2:
            if (now < g_bench_wait_until) {
                return;
            }
            g_bench_phase = BenchPhase::RevFwd;
            break;

        case BenchPhase::RevFwd:
            BusSendRun(RS485_BENCH_ADDR_BROADCAST, 1600U, 1U, 500U, false);
            g_bench_phase = BenchPhase::Wait3;
            g_bench_wait_until = now + 4000U;
            break;

        case BenchPhase::Wait3:
            if (now < g_bench_wait_until) {
                return;
            }
            g_bench_phase = BenchPhase::RevRev;
            break;

        case BenchPhase::RevRev:
            BusSendRun(RS485_BENCH_ADDR_BROADCAST, 1600U, 0U, 500U, false);
            g_bench_phase = BenchPhase::Stop;
            g_bench_wait_until = now + 4000U;
            break;

        case BenchPhase::Stop:
            if (now < g_bench_wait_until) {
                return;
            }
            BusSendStop(RS485_BENCH_ADDR_BROADCAST, false);
            g_bench_phase = BenchPhase::Done;
            break;

        case BenchPhase::Done:
            g_bench_active = false;
            g_bench_phase = BenchPhase::Idle;
            Serial.println(F("Bench complete."));
            PrintStatus();
            break;
    }
}

static bool ParseRunArgs(const char* arg, char* dst_out, uint32_t* steps,
                         uint8_t* dir, uint32_t* hz) {
    while (*arg == ' ') {
        ++arg;
    }

    if (strncmp(arg, "all", 3) == 0 && (arg[3] == '\0' || arg[3] == ' ')) {
        *dst_out = RS485_BENCH_ADDR_BROADCAST;
        arg += 3;
    } else if (arg[0] >= '1' && arg[0] <= '4' &&
               (arg[1] == '\0' || arg[1] == ' ')) {
        *dst_out = arg[0];
        ++arg;
    } else {
        return false;
    }

    while (*arg == ' ') {
        ++arg;
    }

    char* end = nullptr;
    *steps = static_cast<uint32_t>(strtoul(arg, &end, 10));
    if (end == arg) {
        return false;
    }
    arg = end;
    while (*arg == ' ') {
        ++arg;
    }

    const unsigned long dir_val = strtoul(arg, &end, 10);
    if (end == arg || dir_val > 1U) {
        return false;
    }
    *dir = static_cast<uint8_t>(dir_val);
    arg = end;
    while (*arg == ' ') {
        ++arg;
    }

    *hz = static_cast<uint32_t>(strtoul(arg, &end, 10));
    return end != arg && *hz > 0U;
}

static bool ParseJogArgs(const char* arg, char* dst_out, uint8_t* dir, uint32_t* hz) {
    while (*arg == ' ') {
        ++arg;
    }

    if (strncmp(arg, "all", 3) == 0 && (arg[3] == '\0' || arg[3] == ' ')) {
        *dst_out = RS485_BENCH_ADDR_BROADCAST;
        arg += 3;
    } else if (arg[0] >= '1' && arg[0] <= '4' &&
               (arg[1] == '\0' || arg[1] == ' ')) {
        *dst_out = arg[0];
        ++arg;
    } else {
        return false;
    }

    while (*arg == ' ') {
        ++arg;
    }

    char* end = nullptr;
    const unsigned long dir_val = strtoul(arg, &end, 10);
    if (end == arg || dir_val > 1U) {
        return false;
    }
    *dir = static_cast<uint8_t>(dir_val);
    arg = end;
    while (*arg == ' ') {
        ++arg;
    }

    *hz = static_cast<uint32_t>(strtoul(arg, &end, 10));
    return end != arg && *hz > 0U;
}

static bool ParseHoldArgs(const char* arg, char* dst_out, uint8_t* dir) {
    while (*arg == ' ') {
        ++arg;
    }

    if (strncmp(arg, "all", 3) == 0 && (arg[3] == '\0' || arg[3] == ' ')) {
        *dst_out = RS485_BENCH_ADDR_BROADCAST;
        arg += 3;
    } else if (arg[0] >= '1' && arg[0] <= '4' &&
               (arg[1] == '\0' || arg[1] == ' ')) {
        *dst_out = arg[0];
        ++arg;
    } else {
        return false;
    }

    while (*arg == ' ') {
        ++arg;
    }

    char* end = nullptr;
    const unsigned long dir_val = strtoul(arg, &end, 10);
    if (end == arg || dir_val > 1U) {
        return false;
    }
    *dir = static_cast<uint8_t>(dir_val);
    return true;
}

static void HandleCommand(const char* cmd) {
    while (*cmd == ' ') {
        ++cmd;
    }

    if (strcmp(cmd, "help") == 0) {
        PrintHelp();
        return;
    }
    if (strcmp(cmd, "status") == 0) {
        PrintStatus();
        return;
    }
    if (strcmp(cmd, "bench") == 0) {
        StartBench();
        return;
    }
    if (strncmp(cmd, "wave", 4) == 0) {
        const char* arg = cmd + 4;
        while (*arg == ' ') {
            ++arg;
        }
        char dst = RS485_BENCH_ADDR_BROADCAST;
        uint32_t steps = 1000U;
        uint32_t hz = 500U;

        if (*arg != '\0') {
            if (strncmp(arg, "all", 3) == 0 &&
                (arg[3] == '\0' || arg[3] == ' ')) {
                dst = RS485_BENCH_ADDR_BROADCAST;
                arg += 3;
            } else if (arg[0] >= '1' && arg[0] <= '4' &&
                       (arg[1] == '\0' || arg[1] == ' ')) {
                dst = arg[0];
                ++arg;
            }
            // else: first token is steps (legacy: "wave 1000 500" = all)

            while (*arg == ' ') {
                ++arg;
            }
            if (*arg != '\0') {
                char* end = nullptr;
                steps = static_cast<uint32_t>(strtoul(arg, &end, 10));
                if (end == arg || steps == 0U) {
                    Serial.println(
                        F("usage: wave [all|1-4] [steps] [hz]  (def: wave all 1000 500)"));
                    return;
                }
                arg = end;
                while (*arg == ' ') {
                    ++arg;
                }
                if (*arg != '\0') {
                    hz = static_cast<uint32_t>(strtoul(arg, &end, 10));
                    if (end == arg || hz == 0U) {
                        Serial.println(
                            F("usage: wave [all|1-4] [steps] [hz]  (def: wave all 1000 500)"));
                        return;
                    }
                }
            }
        }
        StartWave(dst, steps, hz);
        return;
    }
    if (strcmp(cmd, "stop") == 0 || strcmp(cmd, "stop all") == 0) {
        BusSendStop(RS485_BENCH_ADDR_BROADCAST, false);
        g_bench_active = false;
        g_bench_phase = BenchPhase::Idle;
        g_wave_active = false;
        return;
    }

    if (strncmp(cmd, "run ", 4) == 0) {
        char dst = 0;
        uint32_t steps = 0;
        uint8_t dir = 0;
        uint32_t hz = 0;
        if (!ParseRunArgs(cmd + 4, &dst, &steps, &dir, &hz)) {
            Serial.println(F("usage: run all|<1-4> <steps> <dir> <hz>"));
            return;
        }
        BusSendRun(dst, steps, dir, hz, false);
        return;
    }

    if (strncmp(cmd, "jog ", 4) == 0) {
        char dst = 0;
        uint8_t dir = 0;
        uint32_t hz = 0;
        if (!ParseJogArgs(cmd + 4, &dst, &dir, &hz)) {
            Serial.println(F("usage: jog all|<1-4> <dir> <hz>"));
            return;
        }
        BusSendJog(dst, dir, hz, false);
        return;
    }

    if (strncmp(cmd, "hold ", 5) == 0) {
        char dst = 0;
        uint8_t dir = 0;
        if (!ParseHoldArgs(cmd + 5, &dst, &dir)) {
            Serial.println(F("usage: hold all|<1-4> <0|1>"));
            return;
        }
        BusSendHold(dst, dir, false);
        return;
    }

    if (strncmp(cmd, "ping ", 5) == 0) {
        const char* arg = cmd + 5;
        while (*arg == ' ') {
            ++arg;
        }
        if (strcmp(arg, "all") == 0) {
            SendPing(RS485_BENCH_ADDR_BROADCAST);
        } else if (arg[0] >= '1' && arg[0] <= '4') {
            SendPing(arg[0]);
        } else {
            Serial.println(F("usage: ping <1-4|all>"));
        }
        return;
    }

    Serial.print(F("unknown command: "));
    Serial.println(cmd);
    Serial.println(F("Type 'help' for commands."));
}

void setup() {
    Serial.begin(115200);
    for (uint32_t t = millis(); !Serial && millis() - t < 3000U;) {
        delay(10);
    }

    pinMode(kRs485DePin, OUTPUT);
    SetTransmitMode(true);
    pinMode(kLedPin, OUTPUT);
    digitalWrite(kLedPin, LOW);

    Serial1Obj.begin(RS485_BENCH_BAUD);

    Serial.println();
    Serial.println(F("========================================="));
    Serial.println(F(" RS-485 motor bench — Pico master"));
    Serial.println(F("========================================="));
    Serial.print(F("  Bus baud: "));
    Serial.println(RS485_BENCH_BAUD);
    Serial.println(F("  Type 'help' for commands."));
    Serial.println();
}

void loop() {
    static char cmd_buf[80];
    static uint8_t cmd_len = 0;

    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\r' || c == '\n') {
            if (cmd_len > 0) {
                cmd_buf[cmd_len] = '\0';
                HandleCommand(cmd_buf);
                cmd_len = 0;
            }
        } else if (cmd_len < sizeof(cmd_buf) - 1U) {
            cmd_buf[cmd_len++] = c;
        }
    }

    BenchTick();
    WaveTick();
}
