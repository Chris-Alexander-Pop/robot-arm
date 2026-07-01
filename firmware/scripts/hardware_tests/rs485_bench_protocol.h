// Addressed ASCII framing for multi-node RS-485 bench tests.
//
// Extends rs485_bench_config.h (baud, DE timing, frame-gap constants).
//
// Frame format (human-readable, CR+LF terminated):
//   @<dst> <cmd>\r\n          master -> node(s)
//   @<src> ACK <cmd>\r\n      node -> master reply
//
//   <dst>: '1'..'5'  = single node
//          '*'       = broadcast (all nodes respond)
//   <cmd>: "PING"    = basic reachability ping
//          "ECHO <text>"  = echo payload (corruption test)
//
// Examples:
//   "@2 PING\r\n"       -> only node 2 handles; replies "@2 ACK PING\r\n"
//   "@* PING\r\n"       -> all nodes reply in sequence (master collects)
//
// Used by: rs485_pico/ (Pico master) and rs485_esp32_bench/ (ESP32-C3 slaves).
// Max frame length fits in RS485_BENCH_FRAME_MAX_LEN bytes.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
#include "rs485_bench_config.h"
#endif

// Maximum wire length of one addressed frame (cmd + overhead).
#define RS485_BENCH_FRAME_MAX_LEN 48U

// Special destination for broadcast.
#define RS485_BENCH_ADDR_BROADCAST '*'

#ifdef __cplusplus

namespace rs485_bench {

// --------------------------------------------------------------------------
// Frame construction
// --------------------------------------------------------------------------

// Write "@<dst> <cmd>\r\n" into buf (null-terminated; returns byte count incl. CRLF).
// Returns 0 if buf is too small.
inline size_t BuildFrame(char* buf, size_t buf_size, char dst, const char* cmd) {
    if (buf == nullptr || cmd == nullptr || buf_size < 8U) {
        return 0U;
    }
    // "@X CMD\r\n" — worst case for PING is 11 bytes including NUL
    const int written = snprintf(buf, buf_size, "@%c %s\r\n", dst, cmd);
    if (written <= 0 || static_cast<size_t>(written) >= buf_size) {
        return 0U;
    }
    return static_cast<size_t>(written);
}

inline size_t BuildPingFrame(char* buf, size_t buf_size, char dst) {
    return BuildFrame(buf, buf_size, dst, "PING");
}

// "@<src> ACK PING\r\n"
inline size_t BuildAckPingFrame(char* buf, size_t buf_size, uint8_t src_id) {
    const char src = static_cast<char>('0' + src_id);
    return BuildFrame(buf, buf_size, src, "ACK PING");
}

// --------------------------------------------------------------------------
// Frame parsing
// --------------------------------------------------------------------------

struct ParsedFrame {
    char     dst;          // destination char ('1'..'5' or '*'), 0 if invalid
    char     src;          // source char when parsing an ACK reply, 0 if N/A
    char     cmd[32];      // command string (NUL terminated, up to 31 chars + NUL)
    bool     is_ack;       // true when cmd starts with "ACK "
    bool     valid;        // false if parse failed
};

// Parse a CRLF-terminated frame read from the bus.
// data need not be NUL-terminated; length is provided.
inline ParsedFrame ParseFrame(const uint8_t* data, size_t length) {
    ParsedFrame out{};
    out.valid = false;

    // Minimum: "@X C\r\n" = 6 bytes
    if (data == nullptr || length < 6U) {
        return out;
    }
    if (data[0] != '@') {
        return out;
    }

    const char addr = static_cast<char>(data[1]);
    if (addr != RS485_BENCH_ADDR_BROADCAST &&
        (addr < '1' || addr > '9')) {
        return out;
    }
    if (data[2] != ' ') {
        return out;
    }

    // Extract cmd (strip trailing \r\n)
    size_t cmd_len = length - 3U;
    while (cmd_len > 0U &&
           (data[3U + cmd_len - 1U] == '\r' || data[3U + cmd_len - 1U] == '\n')) {
        --cmd_len;
    }
    if (cmd_len == 0U || cmd_len >= sizeof(out.cmd)) {
        return out;
    }

    memcpy(out.cmd, &data[3], cmd_len);
    out.cmd[cmd_len] = '\0';

    out.dst  = addr;
    out.src  = addr;   // for replies, src == addr field
    out.is_ack = (strncmp(out.cmd, "ACK ", 4) == 0);
    out.valid  = true;
    return out;
}

// Returns true if a frame addressed to this node (or broadcast) should be handled.
inline bool FrameMatchesNode(const ParsedFrame& frame, uint8_t node_id) {
    if (!frame.valid) {
        return false;
    }
    if (frame.dst == RS485_BENCH_ADDR_BROADCAST) {
        return true;
    }
    return frame.dst == static_cast<char>('0' + node_id);
}

// Sanity-check raw bytes before parsing: starts with '@', ends with \n,
// every byte is printable ASCII or \r/\n.
inline bool IsPlausibleFrame(const uint8_t* data, size_t length) {
    if (data == nullptr || length < 6U || length > RS485_BENCH_FRAME_MAX_LEN) {
        return false;
    }
    if (data[0] != '@') {
        return false;
    }
    for (size_t i = 0U; i < length; ++i) {
        const uint8_t b = data[i];
        if (b == '\r' || b == '\n') {
            continue;
        }
        if (b < 0x20U || b > 0x7EU) {
            return false;
        }
    }
    return true;
}

}  // namespace rs485_bench

#endif  // __cplusplus
