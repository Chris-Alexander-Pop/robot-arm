from robot_arm_sim.planning.trapezoidal_profile import generate_profile


def test_profile_has_start_and_end_points() -> None:
    profile = generate_profile([0.0, 0.0], [1.0, 2.0], duration_s=2.0)
    assert len(profile) == 2
    assert profile[0].time_s == 0.0
    assert profile[-1].time_s == 2.0


def test_profile_copies_endpoint_values() -> None:
    start = [0.0, 0.5]
    goal = [1.0, 2.0]

    profile = generate_profile(start, goal, duration_s=2.0)

    start[0] = 9.0
    goal[1] = 9.0

    assert profile[0].joints == [0.0, 0.5]
    assert profile[-1].joints == [1.0, 2.0]
