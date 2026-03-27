import numpy as np

from dh_solver import dh_transformation_matrix


def test_dh_transform_identity_at_zero_parameters() -> None:
    transform = dh_transformation_matrix(0.0, 0.0, 0.0, 0.0)

    expected = np.eye(4)

    assert np.allclose(transform, expected)


def test_dh_transform_applies_rotation_and_translation() -> None:
    transform = dh_transformation_matrix(90.0, 0.0, 150.0, 0.0)

    expected = np.array(
        [
            [1.0, 0.0, 0.0, 0.0],
            [0.0, 0.0, -1.0, 0.0],
            [0.0, 1.0, 0.0, 150.0],
            [0.0, 0.0, 0.0, 1.0],
        ]
    )

    assert np.allclose(transform, expected)


def test_two_link_chain_matches_expected_pose() -> None:
    base_to_link1 = dh_transformation_matrix(0.0, 200.0, 0.0, 0.0)
    link1_to_link2 = dh_transformation_matrix(90.0, 0.0, 150.0, 0.0)

    end_effector = base_to_link1 @ link1_to_link2

    expected = np.array(
        [
            [1.0, 0.0, 0.0, 200.0],
            [0.0, 0.0, -1.0, 0.0],
            [0.0, 1.0, 0.0, 150.0],
            [0.0, 0.0, 0.0, 1.0],
        ]
    )

    assert np.allclose(end_effector, expected)
