#include "control/joint_controller.h"

namespace robot_arm {

namespace {

// P-only starting point: tune on hardware once StepperDriver emits pulses.
constexpr float kDefaultKp = 2.0F;
// Cap commanded joint velocity from the supervisor (deg/s).
constexpr float kMaxSupervisorVelocityDegS = 120.0F;

float SaturateVelocity(float velocity_deg_s) {
  if (velocity_deg_s > kMaxSupervisorVelocityDegS) {
    return kMaxSupervisorVelocityDegS;
  }
  if (velocity_deg_s < -kMaxSupervisorVelocityDegS) {
    return -kMaxSupervisorVelocityDegS;
  }
  return velocity_deg_s;
}

}  // namespace

JointController::JointController(StepperDriver& stepper_driver)
    : stepper_driver_(stepper_driver),
      pid_{PidController(kDefaultKp, 0.0F, 0.0F), PidController(kDefaultKp, 0.0F, 0.0F), PidController(kDefaultKp, 0.0F, 0.0F),
           PidController(kDefaultKp, 0.0F, 0.0F), PidController(kDefaultKp, 0.0F, 0.0F), PidController(kDefaultKp, 0.0F, 0.0F)} {
  DefaultJointMotionLimits(&limits_);
}

void JointController::SetMotionLimits(const JointMotionLimits& limits) {
  limits_ = limits;
}

const JointMotionLimits& JointController::motion_limits() const {
  return limits_;
}

void JointController::SetCommand(const JointCommand& command) {
  command_ = command;
  ClampJointCommand(limits_, &command_);
}

void JointController::UpdateFromSensors(const JointState& measured_state) {
  measured_state_ = measured_state;
}

void JointController::Step(float dt_s) {
  if (dt_s <= 0.0F) {
    for (int joint = 0; joint < kJointCount; ++joint) {
      pid_[joint].Reset();
      stepper_driver_.SetJointVelocityDegS(joint, 0.0F);
    }
    return;
  }

  for (int joint = 0; joint < kJointCount; ++joint) {
    const float error =
        command_.target_position_deg[joint] - measured_state_.position_deg[joint];
    const float velocity_deg_s =
        SaturateVelocity(pid_[joint].Update(error, dt_s));
    stepper_driver_.SetJointVelocityDegS(joint, velocity_deg_s);
  }
}

const JointCommand& JointController::command() const {
  return command_;
}

const JointState& JointController::measured_state() const {
  return measured_state_;
}

}  // namespace robot_arm
