from robot_arm_sim.kinematics.dh_model import DHModel, JointTransform


def test_forward_kinematics_placeholder_returns_identity_matrix() -> None:
    model = DHModel(
        [
            JointTransform(theta=0.0, d=100.0, a=0.0, alpha=90.0),
            JointTransform(theta=0.0, d=0.0, a=200.0, alpha=0.0),
        ]
    )

    transform = model.forward_kinematics([10.0, -20.0])

    assert transform == [
        [1.0, 0.0, 0.0, 0.0],
        [0.0, 1.0, 0.0, 0.0],
        [0.0, 0.0, 1.0, 0.0],
        [0.0, 0.0, 0.0, 1.0],
    ]


def test_forward_kinematics_keeps_declared_joint_table() -> None:
    joints = [JointTransform(theta=15.0, d=25.0, a=50.0, alpha=-90.0)]
    model = DHModel(joints)

    assert model.joints == joints