#include <cassert>
#include <cstring>

#include "protocol/packet_codec.h"

void run_packet_codec_tests() {
  robot_arm::PacketCodec codec;
  robot_arm::JointCommand command{};
  command.target_position_deg[0] = 10.0F;
  command.target_position_deg[1] = -20.5F;
  command.target_position_deg[2] = 30.25F;
  command.target_position_deg[3] = 40.0F;
  command.target_position_deg[4] = 50.0F;
  command.target_position_deg[5] = 60.0F;

  unsigned char buffer[64]{};
  std::memcpy(buffer + 3, command.target_position_deg, sizeof(command.target_position_deg));

  buffer[0] = 0xAAU;
  buffer[1] = 0x55U;
  buffer[2] = 0x10U;
  buffer[27U] = 0x00U;
  for (size_t index = 2U; index < 27U; ++index) {
    buffer[27U] ^= buffer[index];
  }

  robot_arm::JointCommand decoded{};
  assert(codec.DecodeJointCommand(buffer, 28U, &decoded));
  for (int joint = 0; joint < robot_arm::kJointCount; ++joint) {
    assert(command.target_position_deg[joint] == decoded.target_position_deg[joint]);
  }
}

void run_packet_state_encoding_tests() {
  robot_arm::PacketCodec codec;
  robot_arm::JointState state{};
  state.position_deg[0] = 1.0F;
  state.position_deg[1] = 2.0F;
  state.position_deg[2] = 3.0F;
  state.position_deg[3] = 4.0F;
  state.position_deg[4] = 5.0F;
  state.position_deg[5] = 6.0F;
  state.velocity_deg_s[0] = 6.0F;
  state.velocity_deg_s[1] = 5.0F;
  state.velocity_deg_s[2] = 4.0F;
  state.velocity_deg_s[3] = 3.0F;
  state.velocity_deg_s[4] = 2.0F;
  state.velocity_deg_s[5] = 1.0F;

  unsigned char buffer[128]{};
  const size_t encoded = codec.EncodeJointState(state, buffer, sizeof(buffer));

  assert(encoded == 52U);
  assert(buffer[0] == 0xAAU);
  assert(buffer[1] == 0x55U);
  assert(buffer[2] == 0x11U);
}
