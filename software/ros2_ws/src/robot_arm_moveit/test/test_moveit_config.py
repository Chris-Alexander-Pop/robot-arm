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


def test_moveit_controller_mapping_targets_bridge_action():
    package_share = Path(get_package_share_directory('robot_arm_moveit'))
    controllers_file = package_share / 'config' / 'moveit_controllers.yaml'

    controllers = yaml.safe_load(controllers_file.read_text(encoding='utf-8'))

    controller_manager = controllers['moveit_simple_controller_manager']
    arm_controller = controller_manager['arm_controller']

    assert controller_manager['controller_names'] == ['arm_controller']
    assert arm_controller['type'] == 'FollowJointTrajectory'
    assert arm_controller['action_ns'] == 'follow_joint_trajectory'


def test_moveit_joint_limits_have_complete_positive_motion_limits():
    package_share = Path(get_package_share_directory('robot_arm_moveit'))
    joint_limits_file = package_share / 'config' / 'joint_limits.yaml'

    joint_limits = yaml.safe_load(joint_limits_file.read_text(encoding='utf-8'))
    expected_joints = [f'joint{index}' for index in range(1, 7)]

    for joint_name in expected_joints:
        joint_limit = joint_limits['joint_limits'][joint_name]
        assert joint_limit['has_velocity_limits'] is True
        assert joint_limit['has_acceleration_limits'] is True
        assert joint_limit['max_velocity'] > 0.0
        assert joint_limit['max_acceleration'] > 0.0


def test_moveit_launch_descriptions_include_expected_planning_stack():
    package_share = Path(get_package_share_directory('robot_arm_moveit'))
    launch_file = package_share / 'launch' / 'moveit.launch.py'

    launch_text = launch_file.read_text(encoding='utf-8')

    assert 'robot_state_publisher' in launch_text
    assert 'move_group' in launch_text
    assert 'robot_description_semantic' in launch_text
    assert 'robot_description_kinematics' in launch_text
    assert 'robot_description_planning' in launch_text


def test_sim_and_real_launch_files_split_visualization_from_hardware_bridge():
    package_share = Path(get_package_share_directory('robot_arm_moveit'))
    sim_launch = package_share / 'launch' / 'sim.launch.py'
    real_launch = package_share / 'launch' / 'real.launch.py'

    sim_text = sim_launch.read_text(encoding='utf-8')
    real_text = real_launch.read_text(encoding='utf-8')

    assert "package='rviz2'" in sim_text
    assert "package='rviz2'" in real_text
    assert "package='robot_core'" not in sim_text
    assert "package='robot_core'" in real_text
    assert 'hardware_bridge_node' in real_text
    assert 'hardware_bridge_node' not in sim_text
    assert '/dev/ttyUSB0' in real_text
    assert 'poll_period_ms' in real_text
    assert 'heartbeat_period_ms' in real_text
