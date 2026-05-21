#pragma once

#include <stddef.h>
#include <stdint.h>

#include "bus_protocol/bus_commands.h"
#include "bus_protocol/bus_types.h"

namespace robot_arm {
namespace bus {

struct BusFrame {
  uint8_t dst_addr;
  uint8_t src_addr;
  BusCommand command;
  uint8_t payload_len;
  uint8_t payload[kMaxPayloadBytes];
};

class BusFrameCodec {
 public:
  size_t Encode(const BusFrame& frame, uint8_t* out, size_t capacity) const;
  bool Decode(const uint8_t* buffer, size_t length, BusFrame* out) const;

  static uint16_t Crc16(const uint8_t* data, size_t length);
};

}  // namespace bus
}  // namespace robot_arm
