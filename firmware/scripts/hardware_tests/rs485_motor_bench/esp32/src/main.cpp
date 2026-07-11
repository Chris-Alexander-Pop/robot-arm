// RS-485 motor bench slave — ESP32-C3.
//
// Receives addressed bus frames and drives GPIO5/GPIO6 stepper pulses.
// Logs motion events over WiFi (rs485_log_hub.py).

#include <Arduino.h>
#include <Preferences.h>
#include "pinout.h"
#include "motor_driver.h"
#include "wifi_log.h"
#include "rs485_bench_config.h"
#include "rs485_bench_protocol.h"
#include "rs485_motor_protocol.h"

using namespace esp32c3_slave;
using namespace rs485_bench;
using namespace rs485_motor;

#ifndef ROBOT_ARM_MAX_STEP_HZ
#define ROBOT_ARM_MAX_STEP_HZ 2000U
#endif

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
    return 0U;
#endif
}

static uint8_t  g_node_id = 0;
static uint32_t g_ok      = 0;
static uint32_t g_bad     = 0;
static uint32_t g_ignored = 0;

static void SetTransmitMode(bool /*tx*/) {
    // RX-only bench — DE hard-wired to GND.
}

static void DrainRx() {
    const uint32_t deadline = millis() + RS485_BENCH_FRAME_GAP_MS + 2U;
    while (millis() < deadline) {
        while (Serial1.available() > 0) {
            Serial1.read();
        }
        delay(1);
    }
}

static size_t ReadFrame(uint8_t* out, size_t capacity) {
    if (Serial1.available() == 0) {
        return 0U;
    }

    size_t count = 0;
    uint32_t last_byte_ms = millis();

    while (count < capacity) {
        if (Serial1.available() > 0) {
            out[count++] = static_cast<uint8_t>(Serial1.read());
            last_byte_ms = millis();
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

static void HandleMotorCommand(const ParsedFrame& pf) {
    if (IsStopCommand(pf.cmd)) {
        const uint32_t done = MotorStepsCompleted();
        MotorStop();
        WifiLog("MOTOR", "STOP steps_done=%lu", static_cast<unsigned long>(done));
        return;
    }

    uint8_t hold_dir = 0;
    if (ParseHold(pf.cmd, &hold_dir)) {
        MotorHold(hold_dir);
        // Readback: after hold 1, expect step=0 dir=1. If both read 1, pin mux is wrong.
        WifiLog("MOTOR", "HOLD dir=%u step_pin=%d=%d dir_pin=%d=%d",
                static_cast<unsigned>(hold_dir),
                kStepPin, MotorReadStepLevel(),
                kDirPin, MotorReadDirLevel());
        return;
    }

    RunCommand run{};
    if (ParseRun(pf.cmd, &run)) {
        if (MotorRun(run.steps, run.dir, run.hz)) {
            WifiLog("MOTOR", "RUN start steps=%lu dir=%u hz=%lu",
                    static_cast<unsigned long>(run.steps),
                    static_cast<unsigned>(run.dir),
                    static_cast<unsigned long>(run.hz));
        } else {
            WifiLog("ERR", "RUN rejected steps=%lu hz=%lu",
                    static_cast<unsigned long>(run.steps),
                    static_cast<unsigned long>(run.hz));
        }
        return;
    }

    JogCommand jog{};
    if (ParseJog(pf.cmd, &jog)) {
        if (MotorJog(jog.dir, jog.hz)) {
            WifiLog("MOTOR", "JOG start dir=%u hz=%lu",
                    static_cast<unsigned>(jog.dir),
                    static_cast<unsigned long>(jog.hz));
        } else {
            WifiLog("ERR", "JOG rejected dir=%u hz=%lu",
                    static_cast<unsigned>(jog.dir),
                    static_cast<unsigned long>(jog.hz));
        }
        return;
    }

    WifiLog("RX", "unknown cmd (not motor)");
}

static void HandleUsbCommand(const char* cmd) {
    while (*cmd == ' ') {
        ++cmd;
    }

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
        Serial.print(F(" motor="));
        Serial.print(MotorIsRunning() ? "run" : "idle");
        Serial.print(F(" steps="));
        Serial.print(MotorStepsCompleted());
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

void setup() {
    Serial.begin(115200);
    delay(500);

    g_node_id = ResolveNodeId();

    MotorBegin(kStepPin, kDirPin, kStatusLedPin, ROBOT_ARM_MAX_STEP_HZ);

    if (kRs485DePin >= 0) {
        pinMode(kRs485DePin, OUTPUT);
        SetTransmitMode(false);
    }
    Serial1.begin(RS485_BENCH_BAUD, SERIAL_8N1, kRs485RxPin, kRs485TxPin);
    DrainRx();

    Serial.println();
    Serial.println(F("========================================="));
    Serial.print(F(" RS-485 motor bench slave — node id="));
    Serial.println(g_node_id == 0 ? '?' : static_cast<char>('0' + g_node_id));
    Serial.println(F("========================================="));
    if (g_node_id == 0) {
        Serial.println(F("  WARNING: node ID not set! Use: NODE_ID <1-5>"));
    }
    Serial.print(F("  STEP GPIO"));
    Serial.print(kStepPin);
    Serial.print(F("  DIR GPIO"));
    Serial.println(kDirPin);
    Serial.print(F("  Max step rate: "));
    Serial.print(ROBOT_ARM_MAX_STEP_HZ);
    Serial.println(F(" steps/s"));
    Serial.println();

    WifiLogBegin(g_node_id);
}

void loop() {
    WifiLogPoll();

    if (MotorPollDone()) {
        WifiLog("MOTOR", "done steps=%lu",
                static_cast<unsigned long>(MotorStepsCompleted()));
    }

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

    if (g_node_id == 0) {
        return;
    }

    uint8_t rx_buf[RS485_BENCH_FRAME_MAX_LEN + 4U];
    const size_t rx_len = ReadFrame(rx_buf, sizeof(rx_buf));
    if (rx_len == 0) {
        return;
    }

    if (!IsPlausibleFrame(rx_buf, rx_len)) {
        ++g_bad;
        WifiLog("BAD", "len=%u bad=%lu", static_cast<unsigned>(rx_len),
                static_cast<unsigned long>(g_bad));
        return;
    }

    const ParsedFrame pf = ParseFrame(rx_buf, rx_len);
    if (!pf.valid) {
        ++g_bad;
        return;
    }

    if (!FrameMatchesNode(pf, g_node_id)) {
        ++g_ignored;
        return;
    }

    ++g_ok;
    HandleMotorCommand(pf);
}
