#pragma once

// Raspberry Pi Pico RS-485 master pin map.
// Edit these to match your soldered wiring before flashing.
//
// One-way bench: Pico always transmits, ESP always receives.
// Leave DE+RE tied HIGH (or driven by GPIO2 held HIGH) so the Pico
// MAX485 stays in transmit. Do not expect bus ACKs on this side.

namespace pico_master {

// UART0: connects to MAX485/MAX3485 transceiver.
// GPIO0 = TX  -> DI on the module
// GPIO1 = RX  <- RO on the module (unused in one-way TX-only bench)
constexpr int kRs485UartTx = 0;
constexpr int kRs485UartRx = 1;

// Driver-enable: HIGH = transmit. Held HIGH for one-way TX-only bench.
// Wire DE+RE together to this pin (or hard-tie DE+RE to 3V3).
constexpr int kRs485DePin = 2;

// One-way mode: never drop DE to listen; never wait for bus ACKs.
constexpr bool kRs485TxOnly = true;

// USB serial (SerialUSB / Serial) for host commands — always 115200.
// No pin number needed on Pico; it's the built-in USB CDC port.

// On-board LED (GPIO 25) blinks on each TX.
constexpr int kLedPin = 25;

}  // namespace pico_master
