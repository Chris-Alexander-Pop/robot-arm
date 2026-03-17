#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/types.h"

namespace robot_arm {

class PacketCodec {
 public:
  bool DecodeJointCommand(const uint8_t* buffer, size_t length, JointCommand* out) const;
  size_t EncodeJointState(const JointState& state, uint8_t* out, size_t capacity) const;
};

}  // namespace robot_arm
