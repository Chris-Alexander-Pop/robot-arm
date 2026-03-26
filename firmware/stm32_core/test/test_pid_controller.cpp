#include <cassert>

#include "control/pid_controller.h"

void run_pid_controller_tests() {
  robot_arm::PidController controller(2.0F, 0.5F, 0.0F);

  const float first_output = controller.Update(1.0F, 1.0F);
  const float second_output = controller.Update(1.0F, 1.0F);

  assert(first_output == 2.5F);
  assert(second_output == 3.0F);

  robot_arm::PidController reset_controller(1.0F, 1.0F, 0.0F);
  (void)reset_controller.Update(1.0F, 1.0F);
  reset_controller.Reset();

  const float reset_output = reset_controller.Update(1.0F, 1.0F);
  assert(reset_output == 2.0F);
}