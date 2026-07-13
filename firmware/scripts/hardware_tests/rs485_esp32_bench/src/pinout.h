#pragma once

// ESP32-C3 SuperMini RS-485 slave pin map.
// Edit to match your soldered wiring, then recompile.
//
// USB console uses USB-CDC/JTAG (Serial). Bus uses Serial1 (UART1).
// SuperMini (Mischianti pinout): GPIO21 = RXD0, GPIO20 = TXD0.
//
// One-way bench: ESP always receives. Tie DE+RE to GND on the ESP
// MAX485 so RO is enabled. Do NOT leave DE/RE at 5 V — that disables RX.

namespace esp32c3_slave {

// UART1 — RS-485 transceiver (MAX485 / MAX3485)
constexpr int kRs485TxPin = 20;  // DI on module (unused in RX-only)
constexpr int kRs485RxPin = 21;  // RO on module — RXD0
// -1 = RX-only: DE is hard-wired to GND on the module; firmware does not
// drive a DE pin and does not send bus ACKs (logs go over WiFi/USB).
constexpr int kRs485DePin = -1;

constexpr bool kRs485RxOnly = true;

// Status LED — set to -1 to disable.
constexpr int kStatusLedPin = -1;

}  // namespace esp32c3_slave
