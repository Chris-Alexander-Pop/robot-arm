from dataclasses import dataclass
from typing import List


@dataclass
class TrajectoryPoint:
    time_s: float
    joints: List[float]


def generate_profile(start: List[float], goal: List[float], duration_s: float) -> List[TrajectoryPoint]:
    """Return a minimal two-point profile placeholder."""
    # TODO: implement full trapezoidal velocity profile.
    return [
        TrajectoryPoint(time_s=0.0, joints=start),
        TrajectoryPoint(time_s=duration_s, joints=goal),
    ]
