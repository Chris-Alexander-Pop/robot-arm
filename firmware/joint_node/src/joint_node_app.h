#pragma once

#include <stdint.h>

#include "bus_protocol/bus_commands.h"

namespace joint_node {

class JointNodeApp {
 public:
  void Setup(uint8_t node_id);
  void Loop();

 private:
  void HandleSerialConsole();
  void HandleBusFrame();
  void ProcessCommand(robot_arm::bus::BusCommand command, const uint8_t* payload, uint8_t payload_len);
  void ReplyToMaster(robot_arm::bus::BusCommand response_command);

  uint8_t node_id_ = 0U;
  uint32_t last_heartbeat_ms_ = 0U;
  float target_position_deg_ = 0.0f;
  float target_velocity_deg_s_ = 0.0f;
  float gripper_duty_ = 0.0f;
  bool enabled_ = false;
  uint8_t fault_flags_ = 0U;
};

}  // namespace joint_node
