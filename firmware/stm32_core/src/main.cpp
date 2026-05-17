#include <cstdlib>

#include <Arduino.h>

#include "control/joint_controller.h"
#include "core/types.h"
#include "drivers/encoder_driver.h"
#include "drivers/stepper_driver.h"
#include "protocol/packet_codec.h"

// --- Global driver and controller instances ---
// stepper_driver must be declared before joint_controller (constructor takes a reference).
static robot_arm::StepperDriver stepper_driver;
static robot_arm::EncoderDriver encoder_driver;
static robot_arm::JointController joint_controller(stepper_driver);
static robot_arm::PacketCodec packet_codec;

// --- Command framing constants ---
constexpr size_t kCommandFrameSize    = 28U;
constexpr size_t kHeartbeatFrameSize  = 4U;
constexpr size_t kStateFrameSize      = 52U;

constexpr unsigned long kStatePublishPeriodMs = 20UL;
constexpr unsigned long kHeartbeatTimeoutMs   = 2000UL;

static uint8_t command_frame[kCommandFrameSize]{};
static size_t command_frame_size      = 0U;
static unsigned long last_state_publish_ms = 0UL;
static unsigned long last_heartbeat_ms     = 0UL;
static unsigned long last_loop_ms          = 0UL;

#ifdef ROBOT_ARM_RENODE
constexpr size_t kRenodeCommandBufferSize = 32U;
static char renode_command_buffer[kRenodeCommandBufferSize]{};
static size_t renode_command_size = 0U;

static void handle_renode_command_line(const char* line) {
  robot_arm::JointCommand command{};
  command.target_position_deg[0] = static_cast<float>(std::strtod(line, nullptr));
  joint_controller.SetCommand(command);
  Serial.print("Received serial command: ");
  Serial.println(command.target_position_deg[0], 1);
  Serial.print("New target angle: ");
  Serial.println(command.target_position_deg[0], 2);
}
#endif

static void publish_state() {
  const robot_arm::JointState& state = joint_controller.measured_state();
  uint8_t state_frame[kStateFrameSize]{};
  const size_t encoded = packet_codec.EncodeJointState(state, state_frame, sizeof(state_frame));
  if (encoded == kStateFrameSize) {
    Serial.write(state_frame, encoded);
  }
}

void setup() {
  Serial.begin(115200);
  const unsigned long serial_wait_start_ms = millis();
  while (!Serial && (millis() - serial_wait_start_ms) < 2000UL) {
    delay(10);
  }

  Serial.println("STM32 Robot Arm Controller Initializing...");

  stepper_driver.Init();
  encoder_driver.Init();

  last_heartbeat_ms = millis();
  last_loop_ms      = millis();

  Serial.println("STM32 Robot Arm Controller Ready.");
}

void loop() {
  /*
   * MAIN CONTROL LOOP
   * This should run as fast as possible (>1000 Hz) to maintain smooth stepping.
   */

  const unsigned long now_ms = millis();
  const float dt_s = static_cast<float>(now_ms - last_loop_ms) * 0.001F;
  last_loop_ms = now_ms;

  // 1. Receive serial commands and update the joint controller target.
#ifdef ROBOT_ARM_RENODE
  while (Serial.available() > 0) {
    const int byte_value = Serial.read();
    if (byte_value < 0) {
      break;
    }

    const char received_char = static_cast<char>(byte_value);
    if (received_char == '\r') {
      continue;
    }

    if (received_char == '\n') {
      renode_command_buffer[renode_command_size] = '\0';
      if (renode_command_size > 0U) {
        handle_renode_command_line(renode_command_buffer);
      }
      renode_command_size = 0U;
      continue;
    }

    if (renode_command_size + 1U < kRenodeCommandBufferSize) {
      renode_command_buffer[renode_command_size++] = received_char;
    } else {
      renode_command_size = 0U;
    }
  }
#else
  while (Serial.available() > 0) {
    const int byte_value = Serial.read();
    if (byte_value < 0) {
      break;
    }

    if (command_frame_size == 0U && static_cast<uint8_t>(byte_value) != 0xAAU) {
      continue;
    }

    command_frame[command_frame_size++] = static_cast<uint8_t>(byte_value);
    if (command_frame_size == 2U && (command_frame[0] != 0xAAU || command_frame[1] != 0x55U)) {
      command_frame_size = 0U;
      continue;
    }

    if (command_frame_size == 3U) {
      const uint8_t command_id = command_frame[2];
      if (command_id != 0x10U && command_id != 0x12U) {
        command_frame_size = 0U;
      }
      continue;
    }

    if (command_frame_size == kHeartbeatFrameSize && command_frame[2] == 0x12U) {
      if (packet_codec.DecodeHeartbeat(command_frame, kHeartbeatFrameSize)) {
        last_heartbeat_ms = now_ms;
        publish_state();
      }
      command_frame_size = 0U;
      continue;
    }

    if (command_frame_size == kCommandFrameSize) {
      robot_arm::JointCommand decoded_command{};
      if (packet_codec.DecodeJointCommand(command_frame, kCommandFrameSize, &decoded_command)) {
        joint_controller.SetCommand(decoded_command);
        Serial.print("Received target joint 1 angle: ");
        Serial.println(decoded_command.target_position_deg[0]);
      }
      command_frame_size = 0U;
    }
  }
#endif

  // 2. Read feedback / feed measured_state_ for supervisory control & published state.
  //    EncoderDriver stays a stub unless you expose absolute angles to the MCU; CL57T holds its own inner loop.
  //    Targets are clamped in JointController (JointMotionLimits) — Tier 1.5 exercise: tune ranges / homing datum.
  robot_arm::JointState measured{};
  for (int joint = 0; joint < robot_arm::kJointCount; ++joint) {
    measured.position_deg[joint]  = encoder_driver.ReadJointAngleDeg(joint);
    measured.velocity_deg_s[joint] = 0.0F;
  }
  joint_controller.UpdateFromSensors(measured);

  // 3. Supervisory PID → velocities (JointController::Step() is a TODO scaffold until implemented).
  joint_controller.Step(dt_s);

  // 4. Emit STEP/DIR (StepperDriver::Tick() TODO scaffold — Wire AccelStepper or timers per CONTRIBUTING.md).
  stepper_driver.Tick();

  // 5. Heartbeat watchdog: zero the command if the host goes silent.
  if ((now_ms - last_heartbeat_ms) >= kHeartbeatTimeoutMs) {
    joint_controller.SetCommand(robot_arm::JointCommand{});
  }

  // 6. Periodic state publish.
#ifndef ROBOT_ARM_RENODE
  if ((now_ms - last_state_publish_ms) >= kStatePublishPeriodMs) {
    publish_state();
    last_state_publish_ms = now_ms;
  }
#endif
}
