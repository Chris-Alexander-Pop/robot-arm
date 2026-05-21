#pragma once

#include <stdint.h>

namespace robot_arm {
namespace bus {

// Physical bus: RS-485 half-duplex, 921600 baud, 120 Ω termination at base + wrist.
constexpr uint8_t kFrameHeader0 = 0xAAU;
constexpr uint8_t kFrameHeader1 = 0x55U;

constexpr uint8_t kAddrMaster = 0x00U;
constexpr uint8_t kAddrBroadcast = 0xFFU;
constexpr uint8_t kAddrGripper = 0x07U;
constexpr uint8_t kJointNodeCount = 6U;
constexpr uint8_t kBusNodeCount = 7U;  // J1..J6 + gripper

constexpr uint8_t kMaxPayloadBytes = 32U;
constexpr uint8_t kFrameOverheadBytes = 8U;  // hdr(2)+dst+src+cmd+len+crc(2)
constexpr uint8_t kMaxFrameBytes = kFrameOverheadBytes + kMaxPayloadBytes;

constexpr uint32_t kDefaultBaudRate = 921600U;
constexpr uint32_t kWatchdogTimeoutMs = 500U;

inline uint8_t JointIndexToNodeId(int joint_index) {
  return static_cast<uint8_t>(joint_index + 1);
}

inline int NodeIdToJointIndex(uint8_t node_id) {
  if (node_id < 1U || node_id > kJointNodeCount) {
    return -1;
  }
  return static_cast<int>(node_id) - 1;
}

inline bool IsGripperNode(uint8_t node_id) { return node_id == kAddrGripper; }

inline bool IsJointNode(uint8_t node_id) {
  return node_id >= 1U && node_id <= kJointNodeCount;
}

}  // namespace bus
}  // namespace robot_arm
