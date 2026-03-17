from robot_arm_sim.planning.trapezoidal_profile import generate_profile


def test_profile_has_start_and_end_points() -> None:
    profile = generate_profile([0.0, 0.0], [1.0, 2.0], duration_s=2.0)
    assert len(profile) == 2
    assert profile[0].time_s == 0.0
    assert profile[-1].time_s == 2.0
