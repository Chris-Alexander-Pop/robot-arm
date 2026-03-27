#include "drivers/encoder_driver.h"
#include "drivers/stepper_driver.h"
#include "test_harness.h"

void run_driver_scaffold_tests(TestContext& test) {
  robot_arm::StepperDriver stepper_driver;
  test.Check(stepper_driver.Init(), "stepper driver init should succeed");
  test.Check(stepper_driver.initialized(), "stepper driver should report initialized after Init");
  test.CheckFloatEq(0.0F, stepper_driver.joint_velocity_deg_s(0), "stepper driver should start at zero velocity");
  test.CheckFloatEq(0.0F, stepper_driver.joint_velocity_deg_s(5), "stepper driver should start at zero velocity for all joints");

  stepper_driver.SetJointVelocityDegS(0, 0.0F);
  stepper_driver.SetJointVelocityDegS(1, 125.0F);
  stepper_driver.SetJointVelocityDegS(5, -250.0F);
  stepper_driver.SetJointVelocityDegS(-1, 42.0F);
  stepper_driver.SetJointVelocityDegS(6, 99.0F);

  test.CheckFloatEq(0.0F, stepper_driver.joint_velocity_deg_s(0), "stepper joint 0 velocity should stay at the requested value");
  test.CheckFloatEq(125.0F, stepper_driver.joint_velocity_deg_s(1), "stepper joint 1 velocity should stay at the requested value");
  test.CheckFloatEq(-250.0F, stepper_driver.joint_velocity_deg_s(5), "stepper joint 5 velocity should stay at the requested value");
  test.CheckFloatEq(0.0F, stepper_driver.joint_velocity_deg_s(-1), "stepper invalid joint index should be ignored");
  test.CheckFloatEq(0.0F, stepper_driver.joint_velocity_deg_s(6), "stepper invalid upper joint index should be ignored");

  stepper_driver.Tick();

  test.CheckFloatEq(125.0F, stepper_driver.joint_velocity_deg_s(1), "stepper tick should not clear the commanded velocity");

  robot_arm::EncoderDriver encoder_driver;
  test.Check(encoder_driver.Init(), "encoder driver init should succeed");
  test.Check(encoder_driver.initialized(), "encoder driver should report initialized after Init");

  encoder_driver.SetSimulatedJointAngleDegForTest(0, 12.5F);
  encoder_driver.SetSimulatedJointAngleDegForTest(1, -33.25F);
  encoder_driver.SetSimulatedJointAngleDegForTest(5, 178.0F);

  test.CheckFloatEq(12.5F, encoder_driver.simulated_joint_angle_deg(0), "encoder joint 0 simulated angle should be stored");
  encoder_driver.SetSimulatedJointAngleDegForTest(-1, 99.0F);
  encoder_driver.SetSimulatedJointAngleDegForTest(6, 88.0F);
  test.CheckFloatEq(-33.25F, encoder_driver.simulated_joint_angle_deg(1), "encoder joint 1 simulated angle should be stored");
  test.CheckFloatEq(178.0F, encoder_driver.simulated_joint_angle_deg(5), "encoder joint 5 simulated angle should be stored");

  test.CheckFloatEq(12.5F, encoder_driver.ReadJointAngleDeg(0), "encoder read should return the stored joint 0 angle");
  test.CheckFloatEq(0.0F, encoder_driver.simulated_joint_angle_deg(-1), "encoder invalid negative joint index should read as zero");
  test.CheckFloatEq(0.0F, encoder_driver.simulated_joint_angle_deg(6), "encoder invalid upper joint index should read as zero");
  test.CheckFloatEq(-33.25F, encoder_driver.ReadJointAngleDeg(1), "encoder read should return the stored joint 1 angle");
  test.CheckFloatEq(178.0F, encoder_driver.ReadJointAngleDeg(5), "encoder read should return the stored joint 5 angle");

  test.CheckFloatEq(0.0F, encoder_driver.ReadJointAngleDeg(-1), "encoder invalid joint index should still read as zero");
  test.CheckFloatEq(0.0F, encoder_driver.ReadJointAngleDeg(6), "encoder invalid upper joint index should still read as zero");
}