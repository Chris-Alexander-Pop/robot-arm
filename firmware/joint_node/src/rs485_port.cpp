#include "rs485_port.h"

#include <Arduino.h>

#include "pinout.h"

namespace joint_node {

bool Rs485Port::Begin(uint32_t baud_rate) {
  pinMode(kRs485DePin, OUTPUT);
  SetTransmitMode(false);
  Serial2.begin(baud_rate, SERIAL_8N1, kRs485RxPin, kRs485TxPin);
  return true;
}

void Rs485Port::SetTransmitMode(bool transmit) {
  digitalWrite(kRs485DePin, transmit ? HIGH : LOW);
  if (transmit) {
    Serial2.flush();
  }
}

size_t Rs485Port::Write(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0U) {
    return 0U;
  }
  SetTransmitMode(true);
  const size_t written = Serial2.write(data, length);
  Serial2.flush();
  SetTransmitMode(false);
  return written;
}

size_t Rs485Port::Read(uint8_t* out, size_t capacity, uint32_t timeout_ms) {
  if (out == nullptr || capacity == 0U) {
    return 0U;
  }

  const uint32_t deadline = millis() + timeout_ms;
  size_t total = 0U;
  while (millis() < deadline && total < capacity) {
    while (Serial2.available() > 0 && total < capacity) {
      out[total++] = static_cast<uint8_t>(Serial2.read());
    }
    if (total >= capacity) {
      break;
    }
    delay(1);
  }
  return total;
}

}  // namespace joint_node
