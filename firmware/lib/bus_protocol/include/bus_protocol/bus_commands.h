#pragma once

#include <stdint.h>

namespace robot_arm {
namespace bus {

enum class BusCommand : uint8_t {
  kNop = 0x00,
  kSetJointTarget = 0x20,   // position_deg (f32), velocity_deg_s (f32)
  kJointState = 0x21,       // position_deg, velocity_deg_s, fault_flags (u8)
  kEnable = 0x22,           // u8: 1=enable, 0=disable
  kHome = 0x23,
  kHeartbeat = 0x24,        // master broadcast; slaves refresh watchdog
  kSetGripper = 0x25,       // gripper node: duty 0..1 (f32)
  kGripperState = 0x26,     // duty (f32), fault_flags (u8)
  kDiscover = 0x30,         // master -> broadcast; slaves reply with node id
  kDiscoverResp = 0x31,     // payload: node_id, firmware_version (u16)
};

struct JointTargetPayload {
  float position_deg;
  float velocity_deg_s;
};

struct JointStatePayload {
  float position_deg;
  float velocity_deg_s;
  uint8_t fault_flags;
};

struct GripperTargetPayload {
  float duty;  // 0.0 = open, 1.0 = closed
};

struct GripperStatePayload {
  float duty;
  uint8_t fault_flags;
};

constexpr uint8_t kFaultNone = 0x00U;
constexpr uint8_t kFaultDriverAlarm = 0x01U;
constexpr uint8_t kFaultWatchdog = 0x02U;
constexpr uint8_t kFaultLimit = 0x04U;

}  // namespace bus
}  // namespace robot_arm
