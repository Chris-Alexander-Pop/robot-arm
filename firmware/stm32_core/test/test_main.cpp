// Small harness that keeps running so we can collect many failures in one pass.
#include "test_harness.h"

extern void run_driver_scaffold_tests(TestContext& test);
extern void run_joint_limits_tests(TestContext& test);
extern void run_joint_controller_tests(TestContext& test);
extern void run_joint_controller_step_tests(TestContext& test);
extern void run_packet_codec_tests(TestContext& test);
extern void run_pid_controller_tests(TestContext& test);
extern void run_stepper_driver_contract_tests(TestContext& test);
extern void run_bus_frame_codec_tests(TestContext& test);

int main() {
  TestContext test;

  run_driver_scaffold_tests(test);
  run_stepper_driver_contract_tests(test);
  run_joint_limits_tests(test);
  run_packet_codec_tests(test);
  run_bus_frame_codec_tests(test);
  run_pid_controller_tests(test);
  run_joint_controller_tests(test);
  run_joint_controller_step_tests(test);

  if (test.failures > 0) {
    std::printf("%d test failure(s)\n", test.failures);
  }

  return test.failures == 0 ? 0 : 1;
}