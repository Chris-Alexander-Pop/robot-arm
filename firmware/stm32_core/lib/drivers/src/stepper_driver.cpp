#include "drivers/stepper_driver.h"

// TODO(contributor): implement the ARDUINO hardware block below.
// Steps:
//   1. #include <AccelStepper.h> and #include "pinout.h" at the top of this file.
//   2. Inside the #ifdef ARDUINO block, declare one AccelStepper per joint using
//      AccelStepper::DRIVER mode and the kJn STEP/DIR pin pairs from pinout.h.
//   3. Implement Init() to call setMaxSpeed() and setAcceleration() for each stepper.
//   4. Implement SetJointVelocityDegS() to convert deg/s to step/s and call setSpeed().
//      The conversion factor depends on your motor's steps/rev and microstepping setting.
//   5. Implement Tick() to call runSpeed() for every stepper — this must be called
//      every loop iteration for smooth motion.
//
// The non-ARDUINO stub below keeps the native unit tests passing unchanged.

#ifdef ARDUINO

// TODO: declare AccelStepper instances (one per joint) here, e.g.:
//   static AccelStepper stepper_j1(AccelStepper::DRIVER, kJ1StepPin, kJ1DirPin);
//   static AccelStepper stepper_j2(AccelStepper::DRIVER, kJ2StepPin, kJ2DirPin);
//   ...

namespace robot_arm {

bool StepperDriver::Init() {
  // TODO: for each joint, configure max speed and acceleration, e.g.:
  //   stepper_j1.setMaxSpeed(2000.0F);
  //   stepper_j1.setAcceleration(500.0F);
  initialized_ = true;
  return true;
}

void StepperDriver::SetJointVelocityDegS(int joint_index, float velocity_deg_s) {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return;
  }
  joint_velocity_deg_s_[joint_index] = velocity_deg_s;
  // TODO: convert velocity_deg_s to steps/s and call setSpeed() on the
  // corresponding AccelStepper instance, e.g.:
  //   constexpr float kStepsPerDeg = (200.0F * kMicrosteps) / 360.0F;
  //   stepper_j1.setSpeed(velocity_deg_s * kStepsPerDeg);
}

void StepperDriver::Tick() {
  // TODO: call runSpeed() for each stepper instance, e.g.:
  //   stepper_j1.runSpeed();
  //   stepper_j2.runSpeed();
  //   ...
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

#else  // non-ARDUINO stub — used by native unit tests

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
