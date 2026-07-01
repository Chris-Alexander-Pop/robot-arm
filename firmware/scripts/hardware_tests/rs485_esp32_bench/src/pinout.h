#pragma once

// ESP32-C3 RS-485 slave pin map for the bench carrier.
// Edit to match your soldered wiring, then recompile.
//
// ESP32-C3 has UART0 (USB-CDC) and UART1 (hardware).
// UART1 is used for the RS-485 bus (Serial1 in Arduino framework).

namespace esp32c3_slave {

// UART1 — RS-485 transceiver (MAX485 / MAX3485)
constexpr int kRs485TxPin = 4;   // DI on module
constexpr int kRs485RxPin = 5;   // RO on module
constexpr int kRs485DePin = 6;   // DE (HIGH = transmit)

// Status LED — GPIO 8 is the RGB LED on the official devkitm-1.
// On other C3 boards it may be GPIO 2; adjust if yours differs.
constexpr int kStatusLedPin = 8;

}  // namespace esp32c3_slave
