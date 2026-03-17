from dataclasses import dataclass
from typing import List


@dataclass
class JointTransform:
    theta: float
    d: float
    a: float
    alpha: float


class DHModel:
    def __init__(self, joints: List[JointTransform]) -> None:
        self.joints = joints

    def forward_kinematics(self, joint_positions: List[float]) -> List[List[float]]:
        """Return a 4x4 transform matrix placeholder for the end-effector."""
        _ = joint_positions
        # TODO: implement real DH matrix multiplication chain.
        return [
            [1.0, 0.0, 0.0, 0.0],
            [0.0, 1.0, 0.0, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
        ]
