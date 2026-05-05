#pragma once

// Board-level pin assignments for the Nucleo-F401RE.
// See firmware/pinout.md for the full design rationale and open decisions.
//
// Motor drivers: CL57T (NEMA 23, large joints) and CL42T (NEMA 17, small joints).
// Both are closed-loop stepper drivers — they handle encoder feedback internally.
// The STM32 only needs to send STEP/DIR pulses; the driver corrects for missed steps.
//
// TODO(contributor): verify every entry against your specific board wiring before
// hardware bring-up. The values below are candidate allocations, not a final
// electrical drawing. Cross-check against firmware/pinout.md.

#ifdef ARDUINO

// --- Joint STEP / DIR pairs (J1 – J6) ---
// Using STM32 port/pin names as supported by the STM32duino Arduino framework.
// One GPIO pair per joint — STEP triggers a motor step, DIR sets rotation direction.
constexpr int kJ1StepPin = PB0;   constexpr int kJ1DirPin = PB1;
constexpr int kJ2StepPin = PB10;  constexpr int kJ2DirPin = PB11;
constexpr int kJ3StepPin = PB12;  constexpr int kJ3DirPin = PB13;
constexpr int kJ4StepPin = PB14;  constexpr int kJ4DirPin = PB15;
constexpr int kJ5StepPin = PC6;   constexpr int kJ5DirPin = PC7;
constexpr int kJ6StepPin = PC8;   constexpr int kJ6DirPin = PC9;

// --- Driver alarm inputs (CL57T / CL42T ALM outputs) ---
// Each driver has an ALM output that goes active on a fault (overcurrent,
// position error, overheat, etc.).  Connect ALM to an interrupt-capable GPIO.
// TODO(contributor): assign and wire one ALM input per joint, then implement
// fault handling in the firmware state machine.
// constexpr int kJ1AlmPin = ...;
// constexpr int kJ2AlmPin = ...;
// ... (6 total)

// --- Host communication UART (robot link to Raspberry Pi) ---
// TODO(contributor): confirm whether to use UART or USB CDC for the host link.
// If using UART, PA9 (TX) / PA10 (RX) is the recommended candidate (USART1).
constexpr int kHostUartTxPin = PA9;
constexpr int kHostUartRxPin = PA10;

#endif  // ARDUINO
