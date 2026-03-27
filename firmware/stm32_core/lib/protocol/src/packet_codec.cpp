#include "protocol/packet_codec.h"

#include <cstring>

namespace robot_arm {

namespace {

constexpr uint8_t kFrameHeader0 = 0xAAU;
constexpr uint8_t kFrameHeader1 = 0x55U;
constexpr uint8_t kJointCommandId = 0x10U;
constexpr uint8_t kJointStateId = 0x11U;
constexpr uint8_t kHeartbeatId = 0x12U;
constexpr size_t kJointCountBytes = static_cast<size_t>(kJointCount) * sizeof(float);
constexpr size_t kHeartbeatFrameSize = 2U + 1U + 1U;

uint8_t ComputeChecksum(const uint8_t* data, size_t length) {
  uint8_t checksum = 0U;
  for (size_t index = 0; index < length; ++index) {
    checksum ^= data[index];
  }
  return checksum;
}

bool WriteFloatPayload(const float* values, size_t count, uint8_t* out, size_t capacity, uint8_t command_id) {
  const size_t payload_size = count * sizeof(float);
  const size_t frame_size = 2U + 1U + payload_size + 1U;
  if (out == nullptr || capacity < frame_size) {
    return false;
  }

  out[0] = kFrameHeader0;
  out[1] = kFrameHeader1;
  out[2] = command_id;
  std::memcpy(&out[3], values, payload_size);
  out[frame_size - 1U] = ComputeChecksum(&out[2], 1U + payload_size);
  return true;
}

bool ReadFloatPayload(const uint8_t* buffer, size_t length, uint8_t expected_command_id, float* values, size_t count) {
  const size_t payload_size = count * sizeof(float);
  const size_t frame_size = 2U + 1U + payload_size + 1U;
  if (buffer == nullptr || values == nullptr || length != frame_size) {
    return false;
  }

  if (buffer[0] != kFrameHeader0 || buffer[1] != kFrameHeader1 || buffer[2] != expected_command_id) {
    return false;
  }

  if (ComputeChecksum(&buffer[2], 1U + payload_size) != buffer[frame_size - 1U]) {
    return false;
  }

  std::memcpy(values, &buffer[3], payload_size);
  return true;
}

bool ReadEmptyPayload(const uint8_t* buffer, size_t length, uint8_t expected_command_id) {
  if (buffer == nullptr || length != kHeartbeatFrameSize) {
    return false;
  }

  if (buffer[0] != kFrameHeader0 || buffer[1] != kFrameHeader1 || buffer[2] != expected_command_id) {
    return false;
  }

  return ComputeChecksum(&buffer[2], 1U) == buffer[kHeartbeatFrameSize - 1U];
}

}  // namespace

bool PacketCodec::DecodeJointCommand(const uint8_t* buffer, size_t length, JointCommand* out) const {
  return ReadFloatPayload(buffer, length, kJointCommandId, out != nullptr ? out->target_position_deg : nullptr, kJointCount);
}

bool PacketCodec::DecodeHeartbeat(const uint8_t* buffer, size_t length) const {
  return ReadEmptyPayload(buffer, length, kHeartbeatId);
}

size_t PacketCodec::EncodeJointState(const JointState& state, uint8_t* out, size_t capacity) const {
  const size_t payload_size = 2U * kJointCountBytes;
  const size_t frame_size = 2U + 1U + payload_size + 1U;
  if (out == nullptr || capacity < frame_size) {
    return 0U;
  }

  out[0] = kFrameHeader0;
  out[1] = kFrameHeader1;
  out[2] = kJointStateId;
  std::memcpy(&out[3], state.position_deg, kJointCountBytes);
  std::memcpy(&out[3U + kJointCountBytes], state.velocity_deg_s, kJointCountBytes);
  out[frame_size - 1U] = ComputeChecksum(&out[2], 1U + payload_size);
  return frame_size;
}

size_t PacketCodec::EncodeHeartbeat(uint8_t* out, size_t capacity) const {
  if (out == nullptr || capacity < kHeartbeatFrameSize) {
    return 0U;
  }

  out[0] = kFrameHeader0;
  out[1] = kFrameHeader1;
  out[2] = kHeartbeatId;
  out[3] = ComputeChecksum(&out[2], 1U);
  return kHeartbeatFrameSize;
}

}  // namespace robot_arm
