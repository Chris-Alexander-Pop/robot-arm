import importlib.util
from pathlib import Path

import yaml
import xacro
from ament_index_python.packages import get_package_share_directory


def test_moveit_package_contains_expected_files():
    package_share = Path(get_package_share_directory('robot_arm_moveit'))

    expected_files = [
        package_share / 'config' / 'robot_arm.srdf',
        package_share / 'config' / 'kinematics.yaml',
        package_share / 'config' / 'joint_limits.yaml',
        package_share / 'config' / 'ompl_planning.yaml',
        package_share / 'config' / 'moveit_controllers.yaml',
        package_share / 'launch' / 'moveit.launch.py',
        package_share / 'launch' / 'sim.launch.py',
        package_share / 'launch' / 'real.launch.py',
    ]

    for expected_file in expected_files:
        assert expected_file.is_file(), f'Missing expected file: {expected_file}'


def test_robot_description_xacro_is_expandable():
    description_share = Path(get_package_share_directory('robot_description'))
    xacro_file = description_share / 'urdf' / 'mock_arm.urdf.xacro'

    robot_description = xacro.process_file(str(xacro_file)).toxml()

    assert 'base_link' in robot_description
    assert 'link6' in robot_description


def test_srdf_defines_arm_group_and_virtual_joint():
    package_share = Path(get_package_share_directory('robot_arm_moveit'))
    srdf_file = package_share / 'config' / 'robot_arm.srdf'

    srdf_xml = srdf_file.read_text(encoding='utf-8')

    assert '<group name="arm">' in srdf_xml
    assert 'base_link="base_link" tip_link="link6"' in srdf_xml
    assert 'name="world_joint"' in srdf_xml


def test_joint_limits_cover_all_arm_joints():
    package_share = Path(get_package_share_directory('robot_arm_moveit'))
    joint_limits_file = package_share / 'config' / 'joint_limits.yaml'

    joint_limits = yaml.safe_load(joint_limits_file.read_text(encoding='utf-8'))

    assert sorted(joint_limits['joint_limits'].keys()) == [
        'joint1',
        'joint2',
        'joint3',
        'joint4',
        'joint5',
        'joint6',
    ]


def test_launch_description_wires_robot_state_publisher_and_move_group():
    package_share = Path(get_package_share_directory('robot_arm_moveit'))
    launch_file = package_share / 'launch' / 'moveit.launch.py'

    spec = importlib.util.spec_from_file_location(
        'robot_arm_moveit_launch',
        str(launch_file),
    )
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)

    launch_text = launch_file.read_text(encoding='utf-8')

    assert "package='robot_state_publisher'" in launch_text
    assert "package='moveit_ros_move_group'" in launch_text


def test_sim_launch_adds_rviz():
    package_share = Path(get_package_share_directory('robot_arm_moveit'))
    launch_file = package_share / 'launch' / 'sim.launch.py'

    spec = importlib.util.spec_from_file_location(
        'robot_arm_moveit_sim_launch',
        str(launch_file),
    )
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)

    launch_text = launch_file.read_text(encoding='utf-8')

    assert "package='rviz2'" in launch_text


def test_real_launch_adds_rviz():
    package_share = Path(get_package_share_directory('robot_arm_moveit'))
    launch_file = package_share / 'launch' / 'real.launch.py'

    spec = importlib.util.spec_from_file_location(
        'robot_arm_moveit_real_launch',
        str(launch_file),
    )
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)

    launch_text = launch_file.read_text(encoding='utf-8')

    assert "package='rviz2'" in launch_text
