// Shared RS-485 bench-test settings (STM32 hwtest + Arduino echo).
// Keep both projects on the same baud and ping format.
//
// 38400: reliable on Uno (AltSoftSerial), good margin on longer twisted-pair runs.
// Production arm bus uses 921600 on ESP32 hardware UART — not this bench rate.

#pragma once

#define RS485_BENCH_BAUD 38400UL

#define RS485_BENCH_PING "RS485 PING\r\n"
#define RS485_BENCH_PING_LEN 12U

// Inter-byte silence that marks end of one bus frame (~3× 12-byte TX time @ 38400).
#define RS485_BENCH_FRAME_GAP_MS 5U

// DE settle time before/after UART bytes (half-duplex transceiver).
#define RS485_BENCH_DE_SETTLE_US 300U
