#pragma once

// Raspberry Pi Pico RS-485 master pin map.
// Edit these to match your soldered wiring before flashing.

namespace pico_master {

// UART0: connects to MAX485/MAX3485 transceiver.
// GPIO0 = TX  -> DI on the module
// GPIO1 = RX  <- RO on the module
constexpr int kRs485UartTx = 0;
constexpr int kRs485UartRx = 1;

// Driver-enable: HIGH = transmit, LOW = receive.
constexpr int kRs485DePin = 2;

// USB serial (SerialUSB / Serial) for host commands — always 115200.
// No pin number needed on Pico; it's the built-in USB CDC port.

// On-board LED (GPIO 25) blinks on each TX.
constexpr int kLedPin = 25;

}  // namespace pico_master
