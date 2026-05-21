#include "drivers/stepper_driver.h"

#ifdef ARDUINO

#include <AccelStepper.h>
#include <Arduino.h>
#include "../../../src/pinout.h"

namespace {

// ---------------------------------------------------------------------------
// Cross-check against **motor datasheet**, **CL57T microstep setting**, and **CAD**.
//
// | Symbol | Joint | Meaning |
// |--------|-------|---------|
// | kFullStepsPerMotorRev | J1, J2 | Full steps per motor shaft rev (typically 200) |
// | kMicrostepsPerFullStep | J1, J2 | Microstep divisor (e.g. 16 = 1/16) — CL57T DIP / UART |
// | kJ1MotorRevsPerJointRev | J1 | Motor revs per one **output** rev (cycloidal 19:1) |
// | kJ2MotorRevsPerJointRev | J2 | Motor revs per one **output** rev (cycloidal 15:1) |
// | kJ1MaxSpeed… / kJ2MaxSpeed… | each | AccelStepper cap in **microsteps/s** (tune on hardware) |
// | kJ1Acceleration… / kJ2… | each | AccelStepper accel in **microsteps/s²** |
//
// J3–J6: different drivers, ratios, and limits — do not reuse J1/J2 numbers.
// ---------------------------------------------------------------------------

constexpr float kFullStepsPerMotorRev  = 200.0F;
constexpr float kMicrostepsPerFullStep = 16.0F;

constexpr float MicrostepsPerJointDeg(float motor_revs_per_joint_rev) {
  return (kFullStepsPerMotorRev * kMicrostepsPerFullStep * motor_revs_per_joint_rev) / 360.0F;
}

// --- J1 (base) — NEMA 23 + 19:1 cycloidal ----------------------------------------
constexpr float kJ1MotorRevsPerJointRev = 19.0F;
constexpr float kJ1MicrostepsPerDeg     = MicrostepsPerJointDeg(kJ1MotorRevsPerJointRev);
constexpr float kJ1MaxSpeedMicrostepsPerS        = 2500.0F;
constexpr float kJ1AccelerationMicrostepsPerS2   = 800.0F;

// --- J2 (shoulder) — NEMA 23 + 15:1 cycloidal ------------------------------------
constexpr float kJ2MotorRevsPerJointRev = 15.0F;
constexpr float kJ2MicrostepsPerDeg     = MicrostepsPerJointDeg(kJ2MotorRevsPerJointRev);
constexpr float kJ2MaxSpeedMicrostepsPerS        = 2500.0F;
constexpr float kJ2AccelerationMicrostepsPerS2 = 800.0F;

void ConfigureStepperLimits(AccelStepper& stepper, float max_speed_microsteps_per_s,
                            float acceleration_microsteps_per_s2) {
  stepper.setMaxSpeed(max_speed_microsteps_per_s);
  stepper.setAcceleration(acceleration_microsteps_per_s2);
  stepper.setSpeed(0.0F);
}

void ApplyJointVelocityDegS(AccelStepper& stepper, float velocity_deg_s, float microsteps_per_deg) {
  stepper.setSpeed(velocity_deg_s * microsteps_per_deg);
}

}  // namespace

namespace robot_arm {

static AccelStepper g_stepper_j1(AccelStepper::DRIVER, kJ1StepPin, kJ1DirPin);
static AccelStepper g_stepper_j2(AccelStepper::DRIVER, kJ2StepPin, kJ2DirPin);

bool StepperDriver::Init() {
  ConfigureStepperLimits(g_stepper_j1, kJ1MaxSpeedMicrostepsPerS, kJ1AccelerationMicrostepsPerS2);
  ConfigureStepperLimits(g_stepper_j2, kJ2MaxSpeedMicrostepsPerS, kJ2AccelerationMicrostepsPerS2);

  initialized_ = true;
  return true;
}

void StepperDriver::SetJointVelocityDegS(int joint_index, float velocity_deg_s) {
  if (joint_index < 0 || joint_index >= kJointCount) {
    return;
  }
  joint_velocity_deg_s_[joint_index] = velocity_deg_s;

  switch (joint_index) {
    case 0:
      ApplyJointVelocityDegS(g_stepper_j1, velocity_deg_s, kJ1MicrostepsPerDeg);
      break;
    case 1:
      ApplyJointVelocityDegS(g_stepper_j2, velocity_deg_s, kJ2MicrostepsPerDeg);
      break;
    default:
      break;
  }
}

void StepperDriver::Tick() {
  g_stepper_j1.runSpeed();
  g_stepper_j2.runSpeed();
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

#else  // native unit tests — no Arduino / AccelStepper

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

#endif  
