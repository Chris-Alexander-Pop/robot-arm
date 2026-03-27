#include <array>
#include <cstdlib>
#include <Arduino.h>
#include <AccelStepper.h>
#include <Wire.h>
#include <AS5600.h>

#include "drivers/stepper_driver.h"
#include "core/types.h"
#include "protocol/packet_codec.h"

// --- Pin Definitions ---
// Uses AccelStepper::DRIVER (1 pin for step, 1 for dir)
#define MOTOR1_STEP_PIN 2
#define MOTOR1_DIR_PIN  3

// --- Motor & Encoder Objects ---
AccelStepper stepper1(AccelStepper::DRIVER, MOTOR1_STEP_PIN, MOTOR1_DIR_PIN);
AS5600 encoder1(&Wire);

// --- PID control variables ---
float target_angle = 0.0;
float current_angle = 0.0;
float kp = 10.0; // Proportional gain placeholder
robot_arm::PacketCodec packet_codec;
robot_arm::JointState joint_state{};
robot_arm::StepperDriver stepper_driver;

std::array<float, robot_arm::kJointCount> target_joint_positions_deg{};
std::array<float, robot_arm::kJointCount> current_joint_positions_deg{};
std::array<float, robot_arm::kJointCount> current_joint_velocities_deg_s{};

constexpr size_t kCommandFrameSize = 28U;
constexpr size_t kHeartbeatFrameSize = 4U;
constexpr size_t kStateFrameSize = 52U;
constexpr unsigned long kStatePublishPeriodMs = 20UL;
constexpr unsigned long kHeartbeatTimeoutMs = 2000UL;

uint8_t command_frame[kCommandFrameSize]{};
size_t command_frame_size = 0U;
unsigned long last_state_publish_ms = 0UL;
unsigned long last_heartbeat_ms = 0UL;

#ifdef ROBOT_ARM_RENODE
constexpr size_t kRenodeCommandBufferSize = 32U;
char renode_command_buffer[kRenodeCommandBufferSize]{};
size_t renode_command_size = 0U;

void handle_renode_command_line(const char* line) {
  const float parsed_angle = static_cast<float>(std::strtod(line, nullptr));
  target_joint_positions_deg[0] = parsed_angle;
  target_angle = parsed_angle;
  Serial.print("Received serial command: ");
  Serial.println(parsed_angle, 1);
  Serial.print("New target angle: ");
  Serial.println(parsed_angle, 2);
}
#endif

void publish_state() {
  joint_state.position_deg[0] = current_angle;
  joint_state.velocity_deg_s[0] = current_joint_velocities_deg_s[0];
  for (int joint = 1; joint < robot_arm::kJointCount; ++joint) {
    joint_state.position_deg[joint] = target_joint_positions_deg[joint];
    joint_state.velocity_deg_s[joint] = 0.0F;
  }

  uint8_t state_frame[kStateFrameSize]{};
  const size_t encoded = packet_codec.EncodeJointState(joint_state, state_frame, sizeof(state_frame));
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
  Serial.println("STM32 Robot Arm Controller Ready.");
  last_heartbeat_ms = millis();
  stepper_driver.Init();
  
#ifndef ROBOT_ARM_RENODE
  // Initialize I2C for AS5600 encoders (Uses standard SDA/SCL pins)
  Serial.println("Initializing I2C encoder interface.");
  Wire.begin();
  encoder1.begin();
#else
  Serial.println("Renode mode: skipping I2C encoder interface.");
#endif
  
  // Setup Motor Profile
  stepper1.setMaxSpeed(2000.0);
  stepper1.setAcceleration(500.0);
}

void loop() {
  /*
   * MAIN CONTROL LOOP
   * This should run as fast as possible (>1000 Hz) to maintain smooth stepping.
   */

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
        last_heartbeat_ms = millis();
        publish_state();
      }
      command_frame_size = 0U;
      continue;
    }

    if (command_frame_size == kCommandFrameSize) {
      robot_arm::JointCommand decoded_command{};
      if (packet_codec.DecodeJointCommand(command_frame, kCommandFrameSize, &decoded_command)) {
        for (int joint = 0; joint < robot_arm::kJointCount; ++joint) {
          target_joint_positions_deg[joint] = decoded_command.target_position_deg[joint];
        }
        target_angle = target_joint_positions_deg[0];
        Serial.print("Received target joint 1 angle: ");
        Serial.println(target_angle);
      }
      command_frame_size = 0U;
    }
  }
#endif
  
  // 2. Read true position from the magnetic encoder
  // AS5600 returns 0-4095. This scales it to 0-360 degrees.
#ifndef ROBOT_ARM_RENODE
  current_angle = encoder1.readAngle() * (360.0 / 4096.0);
#else
  current_angle = 0.0;
#endif
  current_joint_positions_deg[0] = current_angle;
  for (int joint = 1; joint < robot_arm::kJointCount; ++joint) {
    current_joint_positions_deg[joint] = target_joint_positions_deg[joint];
  }
  
  // 3. Simple P-control effort calculation
  for (int joint = 0; joint < robot_arm::kJointCount; ++joint) {
    float error = target_joint_positions_deg[joint] - current_joint_positions_deg[joint];
    float speed = error * kp;

    if (speed > 2000.0) speed = 2000.0;
    if (speed < -2000.0) speed = -2000.0;

    current_joint_velocities_deg_s[joint] = speed;
    stepper_driver.SetJointVelocityDegS(joint, speed);

    if (joint == 0) {
      target_angle = target_joint_positions_deg[0];
      stepper1.setSpeed(speed);
    }
  }

  const unsigned long now_ms = millis();
#ifndef ROBOT_ARM_RENODE
  if ((now_ms - last_state_publish_ms) >= kStatePublishPeriodMs) {
    publish_state();
    last_state_publish_ms = now_ms;
  }
#endif

  if ((now_ms - last_heartbeat_ms) >= kHeartbeatTimeoutMs) {
    current_joint_velocities_deg_s[0] = 0.0F;
  }
  
  // 4. Output the STEP/DIR signals based on the active speed.
  // This must be called constantly!
  stepper1.runSpeed();
  stepper_driver.Tick();
}
