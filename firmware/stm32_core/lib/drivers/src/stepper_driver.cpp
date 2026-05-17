#include "drivers/stepper_driver.h"

#ifdef ARDUINO

// TODO(contributor): STEP/DIR for CL57T / CL42T (closed-loop servo is inside the motor driver pack).
// AccelStepper is already wired in firmware/stm32_core/platformio.ini as a dependency; numbered checklist:
//
//   1. #include <Arduino.h>, <AccelStepper.h>, then "pinout.h" (MCU pins live under src/).
//   2. Declare one AccelStepper(jointDriverMode, STEP, DIR) per joint — see kJnStepPin / kJnDirPin constants.
//   3. Init(): setMaxSpeed / setAcceleration (derive max steps/s from (200 × microsteps) / 360 × desired °/s).
//   4. SetJointVelocityDegS(): convert °/s → steps/s, call AccelStepper::setSpeed.
//   5. Tick(): call runSpeed() on every joint every main loop iteration.
//

namespace robot_arm {

bool StepperDriver::Init() {
  initialized_ = true;
  return true;
}

void StepperDriver::SetJointVelocityDegS(int joint_index, float velocity_deg_s) {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return;
  }
  joint_velocity_deg_s_[joint_index] = velocity_deg_s;
}

void StepperDriver::Tick() {
}

bool StepperDriver::initialized() const {
  return initialized_;
}

float StepperDriver::joint_velocity_deg_s(int joint_index) const {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return 0.0F;
  }
  return joint_velocity_deg_s_[joint_index];
}

}  // namespace robot_arm

#else  // native unit tests

namespace robot_arm {

bool StepperDriver::Init() {
  initialized_ = true;
  return true;
}

void StepperDriver::SetJointVelocityDegS(int joint_index, float velocity_deg_s) {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return;
  }
  joint_velocity_deg_s_[joint_index] = velocity_deg_s;
}

void StepperDriver::Tick() {
}

bool StepperDriver::initialized() const {
  return initialized_;
}

float StepperDriver::joint_velocity_deg_s(int joint_index) const {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return 0.0F;
  }
  return joint_velocity_deg_s_[joint_index];
}

}  // namespace robot_arm

#endif  // ARDUINO
