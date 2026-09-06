// JointController::Step() behavior tests.
//
// Step() is P-only: kDefaultKp = 2.0F, Ki = 0, Kd = 0, velocity saturated at
// kMaxSupervisorVelocityDegS = 120.0F (joint_controller.cpp).
//
// Run the full suite:
//   cd firmware/stm32_core
//   pio run -e native && .pio/build/native/program

#include <cstdio>
#include <cmath>

#include "control/joint_controller.h"
#include "drivers/stepper_driver.h"
#include "test_harness.h"

void run_joint_controller_step_tests(TestContext& test) {

  // =========================================================================
  // Step() safety — must never corrupt state or crash
  // =========================================================================

  // Step() with a normal dt does not crash.
  {
    robot_arm::StepperDriver stepper;
    robot_arm::JointController ctrl(stepper);
    ctrl.Step(0.01F);
    test.Check(true, "Step(0.01) should not crash");
  }

  // Step() with zero dt does not crash.
  // PidController::Update() already guards dt <= 0 and returns 0.
  {
    robot_arm::StepperDriver stepper;
    robot_arm::JointController ctrl(stepper);
    ctrl.Step(0.0F);
    test.Check(true, "Step(0.0) should not crash");
  }

  // Step() with negative dt does not crash.
  {
    robot_arm::StepperDriver stepper;
    robot_arm::JointController ctrl(stepper);
    ctrl.Step(-0.05F);
    test.Check(true, "Step(-0.05) should not crash");
  }

  // Many consecutive calls do not crash.
  {
    robot_arm::StepperDriver stepper;
    robot_arm::JointController ctrl(stepper);
    for (int i = 0; i < 1000; ++i) {
      ctrl.Step(0.001F);
    }
    test.Check(true, "1000 consecutive Step() calls should not crash");
  }

  // =========================================================================
  // Step() must not corrupt the stored command
  // =========================================================================

  {
    robot_arm::StepperDriver stepper;
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointCommand cmd{};
    cmd.target_position_deg[0] = 12.5F;
    cmd.target_position_deg[1] = -30.0F;
    cmd.target_position_deg[2] =   0.5F;
    cmd.target_position_deg[3] =  42.0F;
    cmd.target_position_deg[4] =  89.0F;
    cmd.target_position_deg[5] = -135.0F;
    ctrl.SetCommand(cmd);

    ctrl.Step(0.01F);

    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      test.CheckFloatEq(cmd.target_position_deg[j],
                        ctrl.command().target_position_deg[j],
                        "Step() must not corrupt the stored command");
    }
  }

  // =========================================================================
  // Step() must not corrupt the stored measured state
  // =========================================================================

  {
    robot_arm::StepperDriver stepper;
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointState state{};
    state.position_deg[0] =  1.0F;   state.velocity_deg_s[0] = 0.1F;
    state.position_deg[1] =  2.0F;   state.velocity_deg_s[1] = 0.2F;
    state.position_deg[2] =  3.0F;   state.velocity_deg_s[2] = 0.3F;
    state.position_deg[3] =  4.0F;   state.velocity_deg_s[3] = 0.4F;
    state.position_deg[4] =  5.0F;   state.velocity_deg_s[4] = 0.5F;
    state.position_deg[5] =  6.0F;   state.velocity_deg_s[5] = 0.6F;
    ctrl.UpdateFromSensors(state);

    ctrl.Step(0.01F);

    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      test.CheckFloatEq(state.position_deg[j],
                        ctrl.measured_state().position_deg[j],
                        "Step() must not corrupt the stored measured position");
      test.CheckFloatEq(state.velocity_deg_s[j],
                        ctrl.measured_state().velocity_deg_s[j],
                        "Step() must not corrupt the stored measured velocity");
    }
  }

  // =========================================================================
  // Zero error produces zero stepper velocity
  //
  // When command and measured positions are both 0 (the defaults), P-error is
  // 0 for every joint. PidController::Update(0, dt) returns 0 regardless of
  // gains, so SetJointVelocityDegS(joint, 0) is called.
  // =========================================================================

  {
    robot_arm::StepperDriver stepper;
    stepper.Init();
    robot_arm::JointController ctrl(stepper);

    // Defaults: command=0, measured=0, so error=0 for all joints.
    ctrl.Step(0.02F);

    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      test.CheckFloatEq(0.0F, stepper.joint_velocity_deg_s(j),
                        "zero error should produce zero stepper velocity");
    }
  }

  // =========================================================================
  // Equal command and measured → zero velocity
  // =========================================================================

  {
    robot_arm::StepperDriver stepper;
    stepper.Init();
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointCommand cmd{};
    robot_arm::JointState meas{};
    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      const float pos = static_cast<float>(j) * 15.0F;
      cmd.target_position_deg[j]  = pos;
      meas.position_deg[j]        = pos;  // equal → error = 0
    }
    ctrl.SetCommand(cmd);
    ctrl.UpdateFromSensors(meas);

    ctrl.Step(0.02F);

    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      test.CheckFloatEq(0.0F, stepper.joint_velocity_deg_s(j),
                        "equal command and measured position should yield zero velocity");
    }
  }

  // =========================================================================
  // Step(dt=0) with any error must not produce nonzero velocity
  // (PID rejects dt<=0 and returns 0; Step also Reset()s).
  // =========================================================================

  {
    robot_arm::StepperDriver stepper;
    stepper.Init();
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointCommand cmd{};
    cmd.target_position_deg[0] = 90.0F;  // non-zero command
    ctrl.SetCommand(cmd);
    // measured stays at 0 → error = 90 for joint 0

    ctrl.Step(0.0F);  // dt = 0, PID must return 0

    test.CheckFloatEq(0.0F, stepper.joint_velocity_deg_s(0),
                      "Step(dt=0) with nonzero error must not produce nonzero velocity");
  }

  // =========================================================================
  // P-only contract (kDefaultKp = 2.0F, kMaxSupervisorVelocityDegS = 120.0F)
  // Integral growth is not implemented (Ki = 0); that test was deleted.
  // =========================================================================

  // Nonzero position error → saturated P velocity.
  // error = 90, kp = 2 → 180, saturate to 120.
  {
    robot_arm::StepperDriver stepper;
    stepper.Init();
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointCommand cmd{};
    cmd.target_position_deg[0] = 90.0F;
    ctrl.SetCommand(cmd);

    ctrl.Step(0.02F);

    test.CheckFloatEq(120.0F, stepper.joint_velocity_deg_s(0),
                      "error 90 deg with kp 2.0 should saturate at 120 deg/s");
  }

  // Negative error produces negative velocity.
  // error = -45, kp = 2 → -90 (below the cap).
  {
    robot_arm::StepperDriver stepper;
    stepper.Init();
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointCommand cmd{};
    cmd.target_position_deg[0] = -45.0F;
    ctrl.SetCommand(cmd);

    ctrl.Step(0.02F);

    test.CheckFloatEq(-90.0F, stepper.joint_velocity_deg_s(0),
                      "error -45 deg with kp 2.0 should command -90 deg/s");
  }

  // Step() commands every joint independently.
  // error_j = (j+1)*10, vel_j = saturate(2 * error_j) → 20,40,60,80,100,120.
  {
    robot_arm::StepperDriver stepper;
    stepper.Init();
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointCommand cmd{};
    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      cmd.target_position_deg[j] = static_cast<float>(j + 1) * 10.0F;
    }
    ctrl.SetCommand(cmd);

    ctrl.Step(0.02F);

    const float expected[] = {20.0F, 40.0F, 60.0F, 80.0F, 100.0F, 120.0F};
    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      test.CheckFloatEq(expected[j], stepper.joint_velocity_deg_s(j),
                        "Step() should command P velocity for every joint with nonzero error");
    }
  }
}
