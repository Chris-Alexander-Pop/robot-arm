#pragma once

#include <stddef.h>
#include <stdint.h>

#include "bus_protocol/bus_commands.h"
#include "bus_protocol/bus_frame_codec.h"
#include "bus_protocol/bus_types.h"
#include "core/types.h"

namespace robot_arm {

// RS-485 bus master on the base STM32 (Nucleo). Polls joint nodes J1..J6 + gripper.
// Hardware UART + DE/RE GPIO — see firmware/pinout.md (bus section).
class BusMaster {
 public:
  struct NodeTelemetry {
    float position_deg;
    float velocity_deg_s;
    uint8_t fault_flags;
    bool online;
  };

  void Begin(uint32_t baud_rate = bus::kDefaultBaudRate);
  void PollCycle();

  bool SetJointTarget(int joint_index, float position_deg, float velocity_deg_s);
  bool BroadcastHeartbeat();
  bool SetGripperDuty(float duty);

  const NodeTelemetry& Node(uint8_t node_id) const;
  void FillJointState(JointState* out) const;

 private:
  bool Transaction(const bus::BusFrame& request, bus::BusFrame* response, uint32_t timeout_ms);
  bool PollNode(uint8_t node_id, const bus::BusFrame& command);

  bus::BusFrameCodec codec_;
  NodeTelemetry nodes_[bus::kBusNodeCount + 1U] = {};
};

}  // namespace robot_arm
