#include <cassert>

#include "protocol/packet_codec.h"

int main() {
  robot_arm::PacketCodec codec;
  robot_arm::JointState state{};
  unsigned char buffer[128]{};

  const size_t encoded = codec.EncodeJointState(state, buffer, sizeof(buffer));
  assert(encoded == 0U);
  return 0;
}
