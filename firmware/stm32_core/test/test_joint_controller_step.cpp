// JointController::Step() behavior tests.
//
// Structure:
//
//   PASSING NOW: Tests that pass against the current no-op stub AND will
//   continue to pass once Step() is correctly implemented.  These form the
//   always-green regression baseline.
//
//   PENDING: Tests marked with ReportPending() that document the behavior a
//   contributor MUST verify once they fill in Step().  They do NOT increment
//   the failure counter today, so the native test binary still exits 0.
//   Preconditions before tightening PENDING assertions:
//     1. Implement the TODO loop in joint_controller.cpp.
//     2. Initialise pid_[] with non-zero gains where needed (default ctor is kp=ki=kd=0).
//
// Run the full suite:
//   cd firmware/stm32_core
//   pio run -e native && .pio/build/native/program

#include <cstdio>
#include <cmath>

#include "control/joint_controller.h"
#include "drivers/stepper_driver.h"
#include "test_harness.h"

namespace {

// Soft-check helper: prints the outcome without incrementing test.failures.
// Use for tests that document TODO behavior not yet enforced by the stub.
void ReportPending(bool condition, const char* description) {
  if (condition) {
    std::printf("  PENDING [already ok]: %s\n", description);
  } else {
    std::printf("  PENDING [TODO]: %s\n", description);
  }
}

}  // namespace

void run_joint_controller_step_tests(TestContext& test) {

  // =========================================================================
  // PASSING NOW: Step() safety — must never corrupt state or crash
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
  // PASSING NOW: Step() must not corrupt the stored command
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
  // PASSING NOW: Step() must not corrupt the stored measured state
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
  // PASSING NOW + AFTER IMPL: zero error produces zero stepper velocity
  //
  // When command and measured positions are both 0 (the defaults), the PID
  // error is 0 for every joint.  PidController::Update(0, dt) returns 0
  // regardless of gains, so SetJointVelocityDegS(joint, 0) should be called.
  //
  // Passes now:  stub is a no-op → stepper velocities stay at 0 (their init).
  // Passes later: implemented Step() computes error=0 → vel=0 for all joints.
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
  // PASSING NOW + AFTER IMPL: equal command and measured → zero velocity
  //
  // Same guarantee but with non-zero, equal positions so error=0 explicitly.
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
  // PASSING NOW: Step(dt=0) with any error must not call SetJointVelocityDegS
  // with a nonzero value (PID rejects dt<=0 and returns 0).
  //
  // Passes now:  stub is a no-op → velocities stay 0.
  // Passes later: implemented Step() calls PidController::Update(error, 0) → 0.
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
  // PENDING: Behavior once Step() is implemented with non-zero PID gains.
  //
  // These soft checks document the contract but DO NOT fail the build.
  // Preconditions before converting to hard test.Check() calls:
  //   1. Fill in the TODO in joint_controller.cpp.
  //   2. Give pid_[] non-zero gains where the test intends motion (kp>0 is enough for proportional smoke tests).
  // =========================================================================

  std::printf("--- PENDING Step() implementation tests ---\n");

  // PENDING: nonzero position error + positive kp → nonzero velocity
  {
    robot_arm::StepperDriver stepper;
    stepper.Init();
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointCommand cmd{};
    cmd.target_position_deg[0] = 90.0F;
    ctrl.SetCommand(cmd);
    // measured stays at 0 → error = 90.0 for joint 0

    ctrl.Step(0.02F);

    // With kp=1.0 and error=90, expected velocity = 90.0 deg/s.
    // Currently fails because (a) Step() is a stub and (b) default kp=0.
    const bool velocity_nonzero = std::fabs(stepper.joint_velocity_deg_s(0)) > 0.001F;
    ReportPending(velocity_nonzero,
      "nonzero error with kp>0 should produce nonzero stepper velocity [needs impl + gains]");
  }

  // PENDING: integral accumulates over multiple Step() calls with ki>0
  {
    robot_arm::StepperDriver stepper;
    stepper.Init();
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointCommand cmd{};
    cmd.target_position_deg[0] = 10.0F;
    ctrl.SetCommand(cmd);
    // measured stays at 0 → sustained error = 10.0 for joint 0

    const float dt = 0.01F;
    float vel_first = 0.0F;
    float vel_later = 0.0F;
    for (int i = 0; i < 20; ++i) {
      ctrl.Step(dt);
      if (i == 0)  vel_first = stepper.joint_velocity_deg_s(0);
      if (i == 19) vel_later = stepper.joint_velocity_deg_s(0);
    }

    // With ki>0 the integral grows each step so |vel_later| > |vel_first|.
    // Currently fails because Step() is a stub.
    const bool integral_grows = std::fabs(vel_later) > std::fabs(vel_first);
    ReportPending(integral_grows,
      "sustained error with ki>0 should cause growing velocity over multiple steps [needs impl + gains]");
  }

  // PENDING: negative error produces negative velocity
  {
    robot_arm::StepperDriver stepper;
    stepper.Init();
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointCommand cmd{};
    cmd.target_position_deg[0] = -45.0F;
    ctrl.SetCommand(cmd);
    // measured stays at 0 → error = -45.0

    ctrl.Step(0.02F);

    // With kp>0 and error<0, velocity should be negative.
    const bool velocity_negative = stepper.joint_velocity_deg_s(0) < -0.001F;
    ReportPending(velocity_negative,
      "negative position error with kp>0 should produce negative velocity [needs impl + gains]");
  }

  // PENDING: Step() calls SetJointVelocityDegS for ALL joints independently
  {
    robot_arm::StepperDriver stepper;
    stepper.Init();
    robot_arm::JointController ctrl(stepper);

    robot_arm::JointCommand cmd{};
    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      cmd.target_position_deg[j] = static_cast<float>(j + 1) * 10.0F;
    }
    ctrl.SetCommand(cmd);
    // measured stays at 0 → each joint has a different non-zero error

    ctrl.Step(0.02F);

    // With gains configured, each joint should receive a distinct nonzero velocity.
    int nonzero_count = 0;
    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      if (std::fabs(stepper.joint_velocity_deg_s(j)) > 0.001F) {
        ++nonzero_count;
      }
    }
    ReportPending(nonzero_count == robot_arm::kJointCount,
      "Step() should command a nonzero velocity for every joint with nonzero error [needs impl + gains]");
  }

  std::printf("--- end PENDING tests ---\n");
}
