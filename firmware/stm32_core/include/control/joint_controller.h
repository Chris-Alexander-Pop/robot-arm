#pragma once

#include "core/types.h"

namespace robot_arm {

class JointController {
 public:
  JointController();

  void SetCommand(const JointCommand& command);
  void UpdateFromSensors(const JointState& measured_state);
  void Step(float dt_s);

  const JointCommand& command() const;
  const JointState& measured_state() const;

 private:
  JointCommand command_{};
  JointState measured_state_{};
};

}  // namespace robot_arm
