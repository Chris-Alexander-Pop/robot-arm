#include "control/pid_controller.h"
#include "test_harness.h"

void run_pid_controller_tests(TestContext& test) {
  robot_arm::PidController controller(2.0F, 0.5F, 0.0F);

  const float first_output = controller.Update(1.0F, 1.0F);
  const float second_output = controller.Update(1.0F, 1.0F);

  test.CheckFloatEq(2.5F, first_output, "pid first output");
  test.CheckFloatEq(3.0F, second_output, "pid integral accumulation");

  robot_arm::PidController reset_controller(1.0F, 1.0F, 0.0F);
  (void)reset_controller.Update(1.0F, 1.0F);
  reset_controller.Reset();

  const float reset_output = reset_controller.Update(1.0F, 1.0F);
  test.CheckFloatEq(2.0F, reset_output, "pid reset should clear integral state");

  robot_arm::PidController guard_controller(3.0F, 0.0F, 1.0F);
  test.CheckFloatEq(0.0F, guard_controller.Update(1.0F, 0.0F), "pid should reject zero dt");
  test.CheckFloatEq(4.0F, guard_controller.Update(1.0F, 1.0F), "pid should keep prior error after rejected step");

  robot_arm::PidController negative_dt_controller(1.0F, 0.0F, 0.0F);
  test.CheckFloatEq(0.0F, negative_dt_controller.Update(1.0F, -0.5F), "pid should reject negative dt");
  test.CheckFloatEq(1.0F, negative_dt_controller.Update(1.0F, 1.0F), "pid should recover after rejected dt");
}
