// StepperDriver interface contract tests.
//
// These tests verify the full behavioral contract that any StepperDriver
// implementation must satisfy — including the ARDUINO hardware backend.
//
// ALL tests in this file pass against the current non-ARDUINO stub.  When a
// contributor fills in the #ifdef ARDUINO block in stepper_driver.cpp, running
// `pio run -e native && .pio/build/native/program` immediately tells them
// whether their implementation preserves this contract.

#include "drivers/stepper_driver.h"
#include "test_harness.h"

void run_stepper_driver_contract_tests(TestContext& test) {

  // --- Initialization state ---

  {
    robot_arm::StepperDriver driver;
    test.Check(!driver.initialized(),
               "stepper driver should NOT be initialized before Init()");
    const bool ok = driver.Init();
    test.Check(ok, "stepper Init() should return true");
    test.Check(driver.initialized(),
               "stepper driver should be initialized after Init()");
  }

  // --- Default velocities after Init() ---

  {
    robot_arm::StepperDriver driver;
    driver.Init();
    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      test.CheckFloatEq(0.0F, driver.joint_velocity_deg_s(j),
                        "all joint velocities should be zero after fresh Init()");
    }
  }

  // --- SetJointVelocityDegS stores and is readable via joint_velocity_deg_s() ---

  {
    robot_arm::StepperDriver driver;
    driver.Init();
    driver.SetJointVelocityDegS(0, 45.0F);
    test.CheckFloatEq(45.0F, driver.joint_velocity_deg_s(0),
                      "joint 0 positive velocity should be stored faithfully");
    driver.SetJointVelocityDegS(3, -120.5F);
    test.CheckFloatEq(-120.5F, driver.joint_velocity_deg_s(3),
                      "joint 3 negative velocity should be stored faithfully");
    driver.SetJointVelocityDegS(5, 360.0F);
    test.CheckFloatEq(360.0F, driver.joint_velocity_deg_s(5),
                      "joint 5 velocity should be stored faithfully");
  }

  // --- Zero velocity is a valid value ---

  {
    robot_arm::StepperDriver driver;
    driver.Init();
    driver.SetJointVelocityDegS(2, 50.0F);
    driver.SetJointVelocityDegS(2, 0.0F);
    test.CheckFloatEq(0.0F, driver.joint_velocity_deg_s(2),
                      "zero velocity should overwrite a previous non-zero value");
  }

  // --- Large velocities are stored faithfully (no clamping in the native stub) ---

  {
    robot_arm::StepperDriver driver;
    driver.Init();
    driver.SetJointVelocityDegS(0, 10000.0F);
    driver.SetJointVelocityDegS(1, -10000.0F);
    test.CheckFloatEq(10000.0F, driver.joint_velocity_deg_s(0),
                      "large positive velocity should be stored faithfully");
    test.CheckFloatEq(-10000.0F, driver.joint_velocity_deg_s(1),
                      "large negative velocity should be stored faithfully");
  }

  // --- Multiple calls: only the most recent value per joint is kept ---

  {
    robot_arm::StepperDriver driver;
    driver.Init();
    driver.SetJointVelocityDegS(4, 10.0F);
    driver.SetJointVelocityDegS(4, 20.0F);
    driver.SetJointVelocityDegS(4, 30.0F);
    test.CheckFloatEq(30.0F, driver.joint_velocity_deg_s(4),
                      "only the latest velocity should be retained per joint");
  }

  // --- Joints are independent: each holds its own velocity ---

  {
    robot_arm::StepperDriver driver;
    driver.Init();
    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      driver.SetJointVelocityDegS(j, static_cast<float>(j + 1) * 11.0F);
    }
    for (int j = 0; j < robot_arm::kJointCount; ++j) {
      test.CheckFloatEq(static_cast<float>(j + 1) * 11.0F,
                        driver.joint_velocity_deg_s(j),
                        "each joint should hold its own independent velocity");
    }
  }

  // --- Out-of-range joint indices: write is silently ignored ---

  {
    robot_arm::StepperDriver driver;
    driver.Init();
    driver.SetJointVelocityDegS(0, 10.0F);
    driver.SetJointVelocityDegS(-1, 999.0F);
    driver.SetJointVelocityDegS(robot_arm::kJointCount, 999.0F);
    driver.SetJointVelocityDegS(100, 999.0F);
    test.CheckFloatEq(10.0F, driver.joint_velocity_deg_s(0),
                      "out-of-range write should not corrupt joint 0 velocity");
  }

  // --- Out-of-range joint indices: read returns 0 ---

  {
    robot_arm::StepperDriver driver;
    driver.Init();
    test.CheckFloatEq(0.0F, driver.joint_velocity_deg_s(-1),
                      "reading with a negative joint index should return 0");
    test.CheckFloatEq(0.0F, driver.joint_velocity_deg_s(robot_arm::kJointCount),
                      "reading with an out-of-range upper index should return 0");
    test.CheckFloatEq(0.0F, driver.joint_velocity_deg_s(100),
                      "reading with a large out-of-range index should return 0");
  }

  // --- Tick() is safe to call repeatedly ---

  {
    robot_arm::StepperDriver driver;
    driver.Init();
    driver.SetJointVelocityDegS(2, 75.0F);
    for (int i = 0; i < 200; ++i) {
      driver.Tick();
    }
    test.Check(true, "Tick() should not crash when called 200 times");
    test.CheckFloatEq(75.0F, driver.joint_velocity_deg_s(2),
                      "Tick() must not clear the commanded velocity");
  }

  // --- Tick() before Init() should not crash ---

  {
    robot_arm::StepperDriver driver;
    driver.Tick();
    test.Check(true, "Tick() before Init() should not crash");
  }

  // --- SetJointVelocityDegS before Init() should not crash ---

  {
    robot_arm::StepperDriver driver;
    driver.SetJointVelocityDegS(0, 5.0F);
    test.Check(true, "SetJointVelocityDegS before Init() should not crash");
  }

  // --- Velocity precision: small fractional values are stored accurately ---

  {
    robot_arm::StepperDriver driver;
    driver.Init();
    driver.SetJointVelocityDegS(1, 0.125F);
    test.CheckFloatEq(0.125F, driver.joint_velocity_deg_s(1),
                      "small fractional velocity should be stored accurately");
    driver.SetJointVelocityDegS(1, -0.0625F);
    test.CheckFloatEq(-0.0625F, driver.joint_velocity_deg_s(1),
                      "small negative fractional velocity should be stored accurately");
  }
}
