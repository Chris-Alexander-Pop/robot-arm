#include "control/pid_controller.h"

namespace robot_arm {

PidController::PidController(float kp, float ki, float kd)
    : kp_(kp), ki_(ki), kd_(kd), integral_(0.0F), previous_error_(0.0F) {}

float PidController::Update(float error, float dt_s) {
  if (dt_s <= 0.0F) {
    return 0.0F;
  }

  integral_ += error * dt_s;
  const float derivative = (error - previous_error_) / dt_s;
  previous_error_ = error;
  return kp_ * error + ki_ * integral_ + kd_ * derivative;
}

void PidController::Reset() {
  integral_ = 0.0F;
  previous_error_ = 0.0F;
}

}  // namespace robot_arm
