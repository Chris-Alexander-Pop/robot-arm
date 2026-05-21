#pragma once

#include <stddef.h>
#include <stdint.h>

namespace joint_node {

class Rs485Port {
 public:
  bool Begin(uint32_t baud_rate);
  size_t Write(const uint8_t* data, size_t length);
  size_t Read(uint8_t* out, size_t capacity, uint32_t timeout_ms);

 private:
  void SetTransmitMode(bool transmit);
};

}  // namespace joint_node
