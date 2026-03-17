from robot_arm_sim.kinematics.dh_model import DHModel, JointTransform


def main() -> None:
    model = DHModel(
        joints=[
            JointTransform(theta=0.0, d=0.1, a=0.0, alpha=1.5708),
            JointTransform(theta=0.0, d=0.0, a=0.2, alpha=0.0),
        ]
    )
    transform = model.forward_kinematics([0.0, 0.0])
    print("Forward kinematics transform:")
    for row in transform:
        print(row)


if __name__ == "__main__":
    main()
