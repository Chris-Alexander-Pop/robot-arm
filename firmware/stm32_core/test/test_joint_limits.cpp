#include "core/joint_limits.h"
#include "core/types.h"
#include "test_harness.h"

void run_joint_limits_tests(TestContext& test) {
  robot_arm::JointMotionLimits limits{};
  robot_arm::DefaultJointMotionLimits(&limits);
  test.CheckFloatEq(-350.0F, limits.min_deg[3], "default JointMotionLimits min bound");
  test.CheckFloatEq(350.0F, limits.max_deg[3], "default JointMotionLimits max bound");

  robot_arm::JointCommand command{};
  command.target_position_deg[0] = 4000.0F;
  command.target_position_deg[5] = -4000.0F;
  robot_arm::ClampJointCommand(limits, &command);
  test.CheckFloatEq(350.0F, command.target_position_deg[0], "clamp saturates positives at max_deg");
  test.CheckFloatEq(-350.0F, command.target_position_deg[5], "clamp saturates negatives at min_deg");

  robot_arm::JointState measured{};
  measured.position_deg[0] = 12.5F;
  measured.position_deg[1] = -5.0F;
  robot_arm::AlignLimitsToMeasured(measured, 25.0F, &limits);
  test.CheckFloatEq(-12.5F, limits.min_deg[0], "align limits expands symmetrically about measured pose");
  test.CheckFloatEq(37.5F, limits.max_deg[0], "align limits expands symmetrically about measured pose (+)");
  test.CheckFloatEq(-30.0F, limits.min_deg[1], "align preserves separate joint datum");
}
