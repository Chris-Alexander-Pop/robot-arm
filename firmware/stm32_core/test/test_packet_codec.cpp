#include <cstddef>
#include <cstdint>
#include <cstring>

#include "protocol/packet_codec.h"
#include "test_harness.h"

namespace {

uint8_t ComputeChecksum(const uint8_t* data, size_t length) {
  uint8_t checksum = 0U;
  for (size_t index = 0; index < length; ++index) {
    checksum ^= data[index];
  }
  return checksum;
}

void FillPacketCommand(robot_arm::JointCommand& command) {
  command.target_position_deg[0] = 10.0F;
  command.target_position_deg[1] = -20.5F;
  command.target_position_deg[2] = 30.25F;
  command.target_position_deg[3] = 40.0F;
  command.target_position_deg[4] = 50.0F;
  command.target_position_deg[5] = 60.0F;
}

void FillPacketState(robot_arm::JointState& state) {
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
}

}  // namespace

void run_packet_codec_tests(TestContext& test) {
  robot_arm::PacketCodec codec;

  robot_arm::JointCommand command{};
  FillPacketCommand(command);

  uint8_t command_buffer[64]{};
  command_buffer[0] = 0xAAU;
  command_buffer[1] = 0x55U;
  command_buffer[2] = 0x10U;
  std::memcpy(&command_buffer[3], command.target_position_deg, sizeof(command.target_position_deg));
  command_buffer[27U] = ComputeChecksum(&command_buffer[2], 1U + sizeof(command.target_position_deg));

  robot_arm::JointCommand decoded{};
  test.Check(codec.DecodeJointCommand(command_buffer, 28U, &decoded), "packet command frame should decode");
  for (int joint = 0; joint < robot_arm::kJointCount; ++joint) {
    test.CheckFloatEq(command.target_position_deg[joint], decoded.target_position_deg[joint], "decoded command joint mismatch");
  }

  robot_arm::JointState state{};
  FillPacketState(state);
  uint8_t state_buffer[128]{};
  const size_t encoded = codec.EncodeJointState(state, state_buffer, sizeof(state_buffer));
  test.Check(encoded == 52U, "joint state frame should have the expected size");
  test.Check(state_buffer[0] == 0xAAU, "joint state header byte 0");
  test.Check(state_buffer[1] == 0x55U, "joint state header byte 1");
  test.Check(state_buffer[2] == 0x11U, "joint state message id");
  test.Check(std::memcmp(&state_buffer[3], state.position_deg, sizeof(state.position_deg)) == 0, "joint state position payload should round-trip");
  test.Check(std::memcmp(&state_buffer[3U + sizeof(state.position_deg)], state.velocity_deg_s, sizeof(state.velocity_deg_s)) == 0, "joint state velocity payload should round-trip");

  uint8_t expected_state_checksum = 0U;
  for (size_t index = 2U; index < encoded - 1U; ++index) {
    expected_state_checksum ^= state_buffer[index];
  }
  test.Check(state_buffer[encoded - 1U] == expected_state_checksum, "joint state checksum should match the payload");

  test.Check(!codec.DecodeJointCommand(nullptr, 28U, &decoded), "decode should reject null buffer");
  test.Check(!codec.DecodeJointCommand(command_buffer, 28U, nullptr), "decode should reject null output");
  test.Check(!codec.DecodeJointCommand(command_buffer, 27U, &decoded), "decode should reject short frames");

  uint8_t bad_header_buffer[64]{};
  std::memcpy(bad_header_buffer, command_buffer, sizeof(command_buffer));
  bad_header_buffer[0] = 0x00U;
  test.Check(!codec.DecodeJointCommand(bad_header_buffer, 28U, &decoded), "decode should reject bad header");

  uint8_t bad_checksum_buffer[64]{};
  std::memcpy(bad_checksum_buffer, command_buffer, sizeof(command_buffer));
  bad_checksum_buffer[27U] ^= 0xFFU;
  test.Check(!codec.DecodeJointCommand(bad_checksum_buffer, 28U, &decoded), "decode should reject bad checksum");

  uint8_t wrong_command_id_buffer[64]{};
  std::memcpy(wrong_command_id_buffer, command_buffer, sizeof(command_buffer));
  wrong_command_id_buffer[2U] = 0x11U;
  test.Check(!codec.DecodeJointCommand(wrong_command_id_buffer, 28U, &decoded), "decode should reject wrong command id");

  test.Check(codec.EncodeJointState(state, nullptr, sizeof(state_buffer)) == 0U, "encode should reject null output");
  test.Check(codec.EncodeJointState(state, state_buffer, 10U) == 0U, "encode should reject undersized capacity");

  uint8_t heartbeat_buffer[8]{};
  const size_t heartbeat_encoded = codec.EncodeHeartbeat(heartbeat_buffer, sizeof(heartbeat_buffer));
  test.Check(heartbeat_encoded == 4U, "heartbeat frame should have the expected size");
  test.Check(heartbeat_buffer[0] == 0xAAU, "heartbeat header byte 0");
  test.Check(heartbeat_buffer[1] == 0x55U, "heartbeat header byte 1");
  test.Check(heartbeat_buffer[2] == 0x12U, "heartbeat message id");
  test.Check(heartbeat_buffer[3] == ComputeChecksum(&heartbeat_buffer[2], 1U), "heartbeat checksum should match the payload");
  test.Check(codec.DecodeHeartbeat(heartbeat_buffer, heartbeat_encoded), "heartbeat frame should decode");
  test.Check(!codec.DecodeHeartbeat(nullptr, heartbeat_encoded), "heartbeat decode should reject null buffer");
  test.Check(!codec.DecodeHeartbeat(heartbeat_buffer, 3U), "heartbeat decode should reject short frames");

  uint8_t bad_heartbeat_header[8]{};
  std::memcpy(bad_heartbeat_header, heartbeat_buffer, heartbeat_encoded);
  bad_heartbeat_header[0U] = 0x00U;
  test.Check(!codec.DecodeHeartbeat(bad_heartbeat_header, heartbeat_encoded), "heartbeat decode should reject bad header");

  uint8_t bad_heartbeat_buffer[8]{};
  std::memcpy(bad_heartbeat_buffer, heartbeat_buffer, heartbeat_encoded);
  bad_heartbeat_buffer[3U] ^= 0xFFU;
  test.Check(!codec.DecodeHeartbeat(bad_heartbeat_buffer, heartbeat_encoded), "heartbeat decode should reject bad checksum");

  test.Check(!codec.DecodeJointCommand(state_buffer, encoded, &decoded), "decode should reject state frames as commands");
  test.Check(!codec.DecodeHeartbeat(state_buffer, encoded), "heartbeat decode should reject state frames");
}
