#include "drivers/encoder_driver.h"

// ARCHITECTURE NOTE — CL57T / CL42T closed-loop stepper drivers:
//
// The CL57T and CL42T handle encoder feedback entirely inside the driver.
// A motor-mounted encoder plugs directly into the driver; the driver corrects
// for position error without the STM32 ever seeing the encoder signal.
// The STM32 only sends STEP/DIR pulses and optionally monitors the ALM output.
//
// As a result, EncoderDriver is intentionally a no-op stub for this hardware:
//   - Init()                         — succeeds immediately, nothing to init
//   - ReadJointAngleDeg()            — returns 0.0 (no MCU-readable position)
//   - SetSimulatedJointAngleDegForTest() — allows native unit tests to inject
//                                      values and exercise the controller path
//
// TODO(contributor): if you later add an independent position sensor to the
// STM32 (e.g. for homing verification or an additional feedback layer), replace
// the ARDUINO block below with the hardware read implementation.  The interface
// is already wired into JointController::UpdateFromSensors() — nothing in
// main.cpp or the controller needs to change.

#ifdef ARDUINO

namespace robot_arm {

bool EncoderDriver::Init() {
  initialized_ = true;
  return true;
}

float EncoderDriver::ReadJointAngleDeg(int joint_index) const {
  (void)joint_index;
  // CL57T / CL42T: position is tracked inside the driver, not readable here.
  return 0.0F;
}

void EncoderDriver::SetSimulatedJointAngleDegForTest(int joint_index, float angle_deg) {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return;
  }
  simulated_joint_angle_deg_[joint_index] = angle_deg;
}

bool EncoderDriver::initialized() const {
  return initialized_;
}

float EncoderDriver::simulated_joint_angle_deg(int joint_index) const {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return 0.0F;
  }
  return simulated_joint_angle_deg_[joint_index];
}

}  // namespace robot_arm

#else  // non-ARDUINO stub — used by native unit tests

namespace robot_arm {

bool EncoderDriver::Init() {
  initialized_ = true;
  return true;
}

float EncoderDriver::ReadJointAngleDeg(int joint_index) const {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return 0.0F;
  }
  return simulated_joint_angle_deg_[joint_index];
}

void EncoderDriver::SetSimulatedJointAngleDegForTest(int joint_index, float angle_deg) {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return;
  }
  simulated_joint_angle_deg_[joint_index] = angle_deg;
}

bool EncoderDriver::initialized() const {
  return initialized_;
}

float EncoderDriver::simulated_joint_angle_deg(int joint_index) const {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return 0.0F;
  }
  return simulated_joint_angle_deg_[joint_index];
}

}  // namespace robot_arm

#endif  // ARDUINO
