// Motor command extensions for RS-485 bench tests.
//
// Builds on rs485_bench_protocol.h (addressed ASCII framing).
//
// Commands:
//   RUN <steps> <dir> <hz>   — run N step pulses at rate (steps/s); dir 0/1
//   STOP                     — halt immediately
//   JOG <dir> <hz>           — continuous jog until STOP
//
// Examples:
//   "@* RUN 3200 1 500\r\n"  — all nodes run 3200 steps forward at 500 steps/s
//   "@2 STOP\r\n"            — stop node 2 only
//   "@* JOG 0 200\r\n"       — all nodes jog reverse at 200 steps/s

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rs485_bench_protocol.h"

#ifdef __cplusplus

namespace rs485_motor {

struct RunCommand {
    uint32_t steps;
    uint8_t  dir;
    uint32_t hz;
    bool     valid;
};

struct JogCommand {
    uint8_t  dir;
    uint32_t hz;
    bool     valid;
};

inline bool ParseRun(const char* cmd, RunCommand* out) {
    if (out == nullptr || cmd == nullptr) {
        return false;
    }
    *out = {};
    if (strncmp(cmd, "RUN ", 4) != 0) {
        return false;
    }
    const char* p = cmd + 4;
    char* end = nullptr;
    const unsigned long steps = strtoul(p, &end, 10);
    if (end == p) {
        return false;
    }
    p = end;
    while (*p == ' ') {
        ++p;
    }
    const unsigned long dir = strtoul(p, &end, 10);
    if (end == p || dir > 1U) {
        return false;
    }
    p = end;
    while (*p == ' ') {
        ++p;
    }
    const unsigned long hz = strtoul(p, &end, 10);
    if (end == p || hz == 0U) {
        return false;
    }
    out->steps = static_cast<uint32_t>(steps);
    out->dir   = static_cast<uint8_t>(dir);
    out->hz    = static_cast<uint32_t>(hz);
    out->valid = true;
    return true;
}

inline bool ParseJog(const char* cmd, JogCommand* out) {
    if (out == nullptr || cmd == nullptr) {
        return false;
    }
    *out = {};
    if (strncmp(cmd, "JOG ", 4) != 0) {
        return false;
    }
    const char* p = cmd + 4;
    char* end = nullptr;
    const unsigned long dir = strtoul(p, &end, 10);
    if (end == p || dir > 1U) {
        return false;
    }
    p = end;
    while (*p == ' ') {
        ++p;
    }
    const unsigned long hz = strtoul(p, &end, 10);
    if (end == p || hz == 0U) {
        return false;
    }
    out->dir   = static_cast<uint8_t>(dir);
    out->hz    = static_cast<uint32_t>(hz);
    out->valid = true;
    return true;
}

inline bool IsStopCommand(const char* cmd) {
    return cmd != nullptr && strcmp(cmd, "STOP") == 0;
}

// HOLD <dir> — drive DIR steady (0/1), STEP low (multimeter bring-up)
inline bool ParseHold(const char* cmd, uint8_t* dir_out) {
    if (cmd == nullptr || dir_out == nullptr) {
        return false;
    }
    if (strncmp(cmd, "HOLD ", 5) != 0) {
        return false;
    }
    const char* p = cmd + 5;
    char* end = nullptr;
    const unsigned long dir = strtoul(p, &end, 10);
    if (end == p || dir > 1U) {
        return false;
    }
    *dir_out = static_cast<uint8_t>(dir);
    return true;
}

inline size_t BuildHoldFrame(char* buf, size_t buf_size, char dst, uint8_t dir) {
    if (buf == nullptr || buf_size < 12U) {
        return 0U;
    }
    char cmd[16];
    const int written = snprintf(cmd, sizeof(cmd), "HOLD %u", static_cast<unsigned>(dir));
    if (written <= 0 || static_cast<size_t>(written) >= sizeof(cmd)) {
        return 0U;
    }
    return rs485_bench::BuildFrame(buf, buf_size, dst, cmd);
}

inline size_t BuildRunCmd(char* buf, size_t buf_size,
                          uint32_t steps, uint8_t dir, uint32_t hz) {
    if (buf == nullptr || buf_size < 16U) {
        return 0U;
    }
    char cmd[36];
    const int written = snprintf(cmd, sizeof(cmd), "RUN %lu %u %lu",
                                 static_cast<unsigned long>(steps),
                                 static_cast<unsigned>(dir),
                                 static_cast<unsigned long>(hz));
    if (written <= 0 || static_cast<size_t>(written) >= sizeof(cmd)) {
        return 0U;
    }
    return rs485_bench::BuildFrame(buf, buf_size, RS485_BENCH_ADDR_BROADCAST, cmd);
}

inline size_t BuildRunFrame(char* buf, size_t buf_size, char dst,
                            uint32_t steps, uint8_t dir, uint32_t hz) {
    if (buf == nullptr || buf_size < 16U) {
        return 0U;
    }
    char cmd[36];
    const int written = snprintf(cmd, sizeof(cmd), "RUN %lu %u %lu",
                                 static_cast<unsigned long>(steps),
                                 static_cast<unsigned>(dir),
                                 static_cast<unsigned long>(hz));
    if (written <= 0 || static_cast<size_t>(written) >= sizeof(cmd)) {
        return 0U;
    }
    return rs485_bench::BuildFrame(buf, buf_size, dst, cmd);
}

inline size_t BuildStopFrame(char* buf, size_t buf_size, char dst) {
    return rs485_bench::BuildFrame(buf, buf_size, dst, "STOP");
}

inline size_t BuildJogFrame(char* buf, size_t buf_size, char dst,
                            uint8_t dir, uint32_t hz) {
    if (buf == nullptr || buf_size < 12U) {
        return 0U;
    }
    char cmd[24];
    const int written = snprintf(cmd, sizeof(cmd), "JOG %u %lu",
                                 static_cast<unsigned>(dir),
                                 static_cast<unsigned long>(hz));
    if (written <= 0 || static_cast<size_t>(written) >= sizeof(cmd)) {
        return 0U;
    }
    return rs485_bench::BuildFrame(buf, buf_size, dst, cmd);
}

}  // namespace rs485_motor

#endif  // __cplusplus
