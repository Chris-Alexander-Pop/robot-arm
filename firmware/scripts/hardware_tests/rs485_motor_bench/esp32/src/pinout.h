#pragma once

// ESP32-C3 SuperMini — RS-485 motor bench slave pin map.

namespace esp32c3_slave {

// Stepper driver (CL57T / CL42T) — PUL+ / DIR+
constexpr int kStepPin = 5;
constexpr int kDirPin  = 6;

// UART1 — RS-485 transceiver (MAX485 / MAX3485)
constexpr int kRs485TxPin = 20;
constexpr int kRs485RxPin = 21;
constexpr int kRs485DePin = -1;
constexpr bool kRs485RxOnly = true;

// SuperMini built-in LED (GPIO8)
constexpr int kStatusLedPin = 8;

}  // namespace esp32c3_slave
