#pragma once

namespace robot_arm {

class PidController {
 public:
  PidController(float kp, float ki, float kd);

  float Update(float error, float dt_s);
  void Reset();

 private:
  float kp_;
  float ki_;
  float kd_;
  float integral_;
  float previous_error_;
};

}  // namespace robot_arm
