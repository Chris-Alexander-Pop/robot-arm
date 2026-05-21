#include "bus/bus_master.h"

#include <cstring>

namespace robot_arm {

namespace {

bus::BusFrame MakeFrame(uint8_t dst, uint8_t src, bus::BusCommand cmd) {
  bus::BusFrame frame;
  frame.dst_addr = dst;
  frame.src_addr = src;
  frame.command = cmd;
  frame.payload_len = 0U;
  return frame;
}

}  // namespace

void BusMaster::Begin(uint32_t baud_rate) {
  (void)baud_rate;
  // TODO: init UART + RS-485 DE pin; 120 Ω termination enabled on base board only
}

void BusMaster::PollCycle() {
  BroadcastHeartbeat();

  bus::BusFrame cmd =
      MakeFrame(bus::kAddrBroadcast, bus::kAddrMaster, bus::BusCommand::kHeartbeat);
  for (uint8_t node_id = 1U; node_id <= bus::kBusNodeCount; ++node_id) {
    PollNode(node_id, cmd);
  }
}

bool BusMaster::BroadcastHeartbeat() {
  bus::BusFrame frame =
      MakeFrame(bus::kAddrBroadcast, bus::kAddrMaster, bus::BusCommand::kHeartbeat);
  bus::BusFrame unused;
  return Transaction(frame, &unused, 2U);
}

bool BusMaster::SetJointTarget(int joint_index, float position_deg, float velocity_deg_s) {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return false;
  }

  const uint8_t node_id = bus::JointIndexToNodeId(joint_index);
  bus::BusFrame request = MakeFrame(node_id, bus::kAddrMaster, bus::BusCommand::kSetJointTarget);
  bus::JointTargetPayload payload{position_deg, velocity_deg_s};
  request.payload_len = static_cast<uint8_t>(sizeof(payload));
  std::memcpy(request.payload, &payload, sizeof(payload));

  bus::BusFrame response;
  if (!Transaction(request, &response, 5U)) {
    return false;
  }

  if (response.command != bus::BusCommand::kJointState ||
      response.payload_len < static_cast<uint8_t>(sizeof(bus::JointStatePayload))) {
    return false;
  }

  bus::JointStatePayload state;
  std::memcpy(&state, response.payload, sizeof(state));
  nodes_[node_id].position_deg = state.position_deg;
  nodes_[node_id].velocity_deg_s = state.velocity_deg_s;
  nodes_[node_id].fault_flags = state.fault_flags;
  nodes_[node_id].online = true;
  return true;
}

bool BusMaster::SetGripperDuty(float duty) {
  bus::BusFrame request =
      MakeFrame(bus::kAddrGripper, bus::kAddrMaster, bus::BusCommand::kSetGripper);
  bus::GripperTargetPayload payload{duty};
  request.payload_len = static_cast<uint8_t>(sizeof(payload));
  std::memcpy(request.payload, &payload, sizeof(payload));

  bus::BusFrame response;
  return Transaction(request, &response, 5U);
}

const BusMaster::NodeTelemetry& BusMaster::Node(uint8_t node_id) const {
  if (node_id > bus::kBusNodeCount) {
    return nodes_[0];
  }
  return nodes_[node_id];
}

void BusMaster::FillJointState(JointState* out) const {
  if (out == nullptr) {
    return;
  }
  for (int joint = 0; joint < kJointCount; ++joint) {
    const uint8_t node_id = bus::JointIndexToNodeId(joint);
    out->position_deg[joint] = nodes_[node_id].position_deg;
    out->velocity_deg_s[joint] = nodes_[node_id].velocity_deg_s;
  }
}

bool BusMaster::Transaction(const bus::BusFrame& request, bus::BusFrame* response,
                            uint32_t timeout_ms) {
  (void)request;
  (void)response;
  (void)timeout_ms;
  // TODO: drive RS-485 transceiver, write frame, turn around, read response
  return false;
}

bool BusMaster::PollNode(uint8_t node_id, const bus::BusFrame& command) {
  bus::BusFrame request = command;
  request.dst_addr = node_id;
  bus::BusFrame response;
  if (!Transaction(request, &response, 5U)) {
    nodes_[node_id].online = false;
    return false;
  }
  nodes_[node_id].online = true;
  return true;
}

}  // namespace robot_arm
