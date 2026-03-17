#include "protocol/packet_codec.h"

namespace robot_arm {

bool PacketCodec::DecodeJointCommand(const uint8_t* buffer, size_t length, JointCommand* out) const {
  (void)buffer;
  (void)length;
  (void)out;
  // TODO: validate framing/checksum and decode 6 joint targets.
  return false;
}

size_t PacketCodec::EncodeJointState(const JointState& state, uint8_t* out, size_t capacity) const {
  (void)state;
  (void)out;
  (void)capacity;
  // TODO: serialize joint state into protocol packet format.
  return 0U;
}

}  // namespace robot_arm
