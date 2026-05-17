#include "control/joint_controller.h"
#include "core/joint_limits.h"
#include "drivers/stepper_driver.h"
#include "test_harness.h"

namespace {

void FillJointCommand(robot_arm::JointCommand& command) {
  command.target_position_deg[0] = 12.5F;
  command.target_position_deg[1] = -10.0F;
  command.target_position_deg[2] = 0.5F;
  command.target_position_deg[3] = 42.0F;
  command.target_position_deg[4] = 89.0F;
  command.target_position_deg[5] = -135.0F;
}

void FillJointState(robot_arm::JointState& state) {
  state.position_deg[0] = 1.0F;
  state.position_deg[1] = 2.0F;
  state.position_deg[2] = 3.0F;
  state.position_deg[3] = 4.0F;
  state.position_deg[4] = 5.0F;
  state.position_deg[5] = 6.0F;
  state.velocity_deg_s[0] = 0.1F;
  state.velocity_deg_s[1] = 0.2F;
  state.velocity_deg_s[2] = 0.3F;
  state.velocity_deg_s[3] = 0.4F;
  state.velocity_deg_s[4] = 0.5F;
  state.velocity_deg_s[5] = 0.6F;
}

}  // namespace

void run_joint_controller_tests(TestContext& test) {
  robot_arm::StepperDriver stepper_driver;
  robot_arm::JointController controller(stepper_driver);

  const robot_arm::JointCommand& default_command = controller.command();
  const robot_arm::JointState& default_state = controller.measured_state();
  for (int joint = 0; joint < robot_arm::kJointCount; ++joint) {
    test.CheckFloatEq(0.0F, default_command.target_position_deg[joint], "joint controller default command should be zeroed");
    test.CheckFloatEq(0.0F, default_state.position_deg[joint], "joint controller default position should be zeroed");
    test.CheckFloatEq(0.0F, default_state.velocity_deg_s[joint], "joint controller default velocity should be zeroed");
  }

  robot_arm::JointCommand command{};
  FillJointCommand(command);
  const robot_arm::JointCommand expected_command = command;
  controller.SetCommand(command);
  command.target_position_deg[0] = 999.0F;
  for (int joint = 0; joint < robot_arm::kJointCount; ++joint) {
    test.CheckFloatEq(expected_command.target_position_deg[joint], controller.command().target_position_deg[joint], "joint controller should store the latest command");
  }

  robot_arm::JointState measured_state{};
  FillJointState(measured_state);
  const robot_arm::JointState expected_measured_state = measured_state;
  controller.UpdateFromSensors(measured_state);
  measured_state.position_deg[0] = 999.0F;
  measured_state.velocity_deg_s[0] = 999.0F;
  for (int joint = 0; joint < robot_arm::kJointCount; ++joint) {
    test.CheckFloatEq(expected_measured_state.position_deg[joint], controller.measured_state().position_deg[joint], "joint controller should store measured position");
    test.CheckFloatEq(expected_measured_state.velocity_deg_s[joint], controller.measured_state().velocity_deg_s[joint], "joint controller should store measured velocity");
  }

  controller.Step(0.01F);
  for (int joint = 0; joint < robot_arm::kJointCount; ++joint) {
    test.CheckFloatEq(expected_command.target_position_deg[joint], controller.command().target_position_deg[joint], "joint controller step should not clear the command yet");
    test.CheckFloatEq(expected_measured_state.position_deg[joint], controller.measured_state().position_deg[joint], "joint controller step should preserve measured position");
  }

  controller.Step(0.0F);

  // Motion-limit clamping (anti cable-wrap backstop vs runaway winding).
  {
    robot_arm::JointMotionLimits narrow{};
    robot_arm::DefaultJointMotionLimits(&narrow);
    narrow.min_deg[0] = -40.0F;
    narrow.max_deg[0]  = -20.0F;

    robot_arm::JointController limited_controller(stepper_driver);
    limited_controller.SetMotionLimits(narrow);

    robot_arm::JointCommand command{};
    command.target_position_deg[0] = 500.0F;
    limited_controller.SetCommand(command);
    test.CheckFloatEq(-20.0F, limited_controller.command().target_position_deg[0],
                      "motion limits clamp high-setpoint joints to configured max_deg");

    command.target_position_deg[0] = -999.0F;
    limited_controller.SetCommand(command);
    test.CheckFloatEq(-40.0F, limited_controller.command().target_position_deg[0],
                      "motion limits clamp low-setpoint joints to configured min_deg");

    command.target_position_deg[0] = -30.0F;
    limited_controller.SetCommand(command);
    test.CheckFloatEq(-30.0F, limited_controller.command().target_position_deg[0],
                      "motion limits leave in-range targets unchanged");
  }
}
