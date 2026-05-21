#include "joint_node_app.h"

#include <Arduino.h>
#include <Preferences.h>
#include <cstring>

#include "bus_protocol/bus_commands.h"
#include "bus_protocol/bus_frame_codec.h"
#include "bus_protocol/bus_types.h"
#include "node_config.h"
#include "pinout.h"
#include "rs485_port.h"

namespace joint_node {

namespace {

using robot_arm::bus::BusCommand;
using robot_arm::bus::BusFrame;
using robot_arm::bus::BusFrameCodec;
using robot_arm::bus::GripperStatePayload;
using robot_arm::bus::GripperTargetPayload;
using robot_arm::bus::JointStatePayload;
using robot_arm::bus::JointTargetPayload;
using robot_arm::bus::kAddrBroadcast;
using robot_arm::bus::kAddrMaster;
using robot_arm::bus::kDefaultBaudRate;
using robot_arm::bus::kFaultWatchdog;
using robot_arm::bus::kMaxFrameBytes;
using robot_arm::bus::kWatchdogTimeoutMs;

Rs485Port g_rs485;
BusFrameCodec g_codec;
uint8_t g_rx_buffer[kMaxFrameBytes];

constexpr size_t kJointTargetPayloadSize = sizeof(JointTargetPayload);
constexpr size_t kJointStatePayloadSize = sizeof(JointStatePayload);
constexpr size_t kGripperTargetPayloadSize = sizeof(GripperTargetPayload);
constexpr size_t kGripperStatePayloadSize = sizeof(GripperStatePayload);

bool PayloadEquals(const uint8_t* payload, uint8_t len, size_t expected) {
  return payload != nullptr && len == expected;
}

}  // namespace

void JointNodeApp::Setup(uint8_t node_id) {
  node_id_ = node_id;
  pinMode(kStatusLedPin, OUTPUT);
  pinMode(kEnablePin, OUTPUT);
  pinMode(kAlarmPin, INPUT_PULLUP);
  pinMode(kHomePin, INPUT_PULLUP);

  if (IsGripperNode(node_id_)) {
    pinMode(kGripperPwmPin, OUTPUT);
  }

  digitalWrite(kEnablePin, HIGH);  // Active-low enable assumed — HIGH = disabled
  enabled_ = false;
  last_heartbeat_ms_ = millis();

  g_rs485.Begin(kDefaultBaudRate);

  Serial.printf("[joint_node] id=%u role=%s\n", node_id_,
                IsGripperNode(node_id_) ? "gripper" : "joint");
  Serial.println("Console: NODE_ID <1-7>  |  STATUS");
}

void JointNodeApp::Loop() {
  HandleSerialConsole();

  if (enabled_ && (millis() - last_heartbeat_ms_) > kWatchdogTimeoutMs) {
    fault_flags_ |= kFaultWatchdog;
    enabled_ = false;
    digitalWrite(kEnablePin, HIGH);
  }

  if (digitalRead(kAlarmPin) == LOW) {
    fault_flags_ |= robot_arm::bus::kFaultDriverAlarm;
    enabled_ = false;
    digitalWrite(kEnablePin, HIGH);
  }

  HandleBusFrame();
  // TODO: timer-driven STEP/DIR generation and homing state machine
}

void JointNodeApp::HandleSerialConsole() {
  if (!Serial.available()) {
    return;
  }

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0U) {
    return;
  }

  if (line.startsWith("NODE_ID")) {
    const int requested = line.substring(8).toInt();
    if (requested >= 1 && requested <= static_cast<int>(robot_arm::bus::kBusNodeCount)) {
      node_id_ = static_cast<uint8_t>(requested);
      Preferences prefs;
      if (prefs.begin("robot_arm", false)) {
        prefs.putUInt("node_id", static_cast<uint32_t>(node_id_));
        prefs.end();
      }
      Serial.printf("node_id set to %u (stored in NVS)\n", node_id_);
    } else {
      Serial.println("usage: NODE_ID <1-7>");
    }
    return;
  }

  if (line == "STATUS") {
    Serial.printf("id=%u enabled=%d fault=0x%02X target=%.2f vel=%.2f grip=%.2f\n", node_id_,
                  enabled_, fault_flags_, target_position_deg_, target_velocity_deg_s_, gripper_duty_);
  }
}

void JointNodeApp::HandleBusFrame() {
  const size_t received = g_rs485.Read(g_rx_buffer, kMaxFrameBytes, 5U);
  if (received < robot_arm::bus::kFrameOverheadBytes) {
    return;
  }

  BusFrame frame;
  if (!g_codec.Decode(g_rx_buffer, received, &frame)) {
    return;
  }

  if (frame.dst_addr != node_id_ && frame.dst_addr != kAddrBroadcast) {
    return;
  }

  ProcessCommand(frame.command, frame.payload, frame.payload_len);

  if (IsGripperNode(node_id_)) {
    ReplyToMaster(BusCommand::kGripperState);
  } else {
    ReplyToMaster(BusCommand::kJointState);
  }
}

void JointNodeApp::ProcessCommand(BusCommand command, const uint8_t* payload, uint8_t payload_len) {
  switch (command) {
    case BusCommand::kHeartbeat:
      last_heartbeat_ms_ = millis();
      fault_flags_ &= static_cast<uint8_t>(~kFaultWatchdog);
      break;

    case BusCommand::kSetJointTarget:
      if (!IsGripperNode(node_id_) && PayloadEquals(payload, payload_len, kJointTargetPayloadSize)) {
        JointTargetPayload target;
        std::memcpy(&target, payload, kJointTargetPayloadSize);
        target_position_deg_ = target.position_deg;
        target_velocity_deg_s_ = target.velocity_deg_s;
      }
      break;

    case BusCommand::kSetGripper:
      if (IsGripperNode(node_id_) && PayloadEquals(payload, payload_len, kGripperTargetPayloadSize)) {
        GripperTargetPayload target;
        std::memcpy(&target, payload, kGripperTargetPayloadSize);
        gripper_duty_ = target.duty;
        // TODO: map duty to 50 Hz servo PWM on kGripperPwmPin
      }
      break;

    case BusCommand::kEnable:
      if (payload_len >= 1U) {
        enabled_ = payload[0] != 0U;
        digitalWrite(kEnablePin, enabled_ ? LOW : HIGH);
      }
      break;

    case BusCommand::kHome:
      if (!IsGripperNode(node_id_)) {
        // TODO: homing state machine using kHomePin
      }
      break;

    default:
      break;
  }
}

void JointNodeApp::ReplyToMaster(BusCommand response_command) {
  BusFrame response;
  response.dst_addr = kAddrMaster;
  response.src_addr = node_id_;
  response.command = response_command;

  if (response_command == BusCommand::kGripperState) {
    response.payload_len = static_cast<uint8_t>(kGripperStatePayloadSize);
    GripperStatePayload state{gripper_duty_, fault_flags_};
    std::memcpy(response.payload, &state, kGripperStatePayloadSize);
  } else {
    response.payload_len = static_cast<uint8_t>(kJointStatePayloadSize);
    JointStatePayload state{target_position_deg_, target_velocity_deg_s_, fault_flags_};
    std::memcpy(response.payload, &state, kJointStatePayloadSize);
  }

  uint8_t tx[kMaxFrameBytes];
  const size_t encoded = g_codec.Encode(response, tx, kMaxFrameBytes);
  if (encoded > 0U) {
    g_rs485.Write(tx, encoded);
  }
}

}  // namespace joint_node
