#include "drivers/stepper_driver.h"

#ifdef ARDUINO

// TODO(contributor): STEP/DIR for CL57T / CL42T (closed-loop servo is inside the motor driver pack).
// laurb9/StepperDriver (MIT) is in platformio.ini; see hwtest_stepper_*.cpp for usage patterns.
//
//   1. #include <BasicStepperDriver.h> and "pinout.h".
//   2. One BasicStepperDriver(200, DIR, STEP) per joint — note DIR before STEP in the constructor.
//   3. Init(): begin(rpm, microsteps); setSpeedProfile(CONSTANT_SPEED).
//   4. SetJointVelocityDegS(): convert °/s → steps/s → RPM; startMove / nextAction in Tick().
//   5. Tick(): call nextAction() on each active joint every main loop iteration.
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
