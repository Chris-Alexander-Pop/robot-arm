#pragma once

namespace pico_master {

constexpr int kRs485UartTx = 0;
constexpr int kRs485UartRx = 1;
constexpr int kRs485DePin  = 2;
constexpr bool kRs485TxOnly = true;
constexpr int kLedPin = 25;

// Local J0 motor (direct STEP/DIR from Pico — board silk pins 21 / 22).
// Pin 21 = GP16 → PUL+/STEP, pin 22 = GP17 → DIR+.
constexpr int kJ0StepPin = 16;
constexpr int kJ0DirPin  = 17;
constexpr uint32_t kJ0MaxHz = 3000U;

// Destination char used in command parsing for the on-Pico motor.
constexpr char kJ0Addr = '0';

}  // namespace pico_master
