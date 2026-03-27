import os

import xacro
import yaml
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def _load_text(path):
    with open(path, 'r', encoding='utf-8') as file_handle:
        return file_handle.read()


def _load_yaml(path):
    with open(path, 'r', encoding='utf-8') as file_handle:
        return yaml.safe_load(file_handle)


def generate_launch_description():
    description_share = get_package_share_directory('robot_description')
    moveit_share = get_package_share_directory('robot_arm_moveit')

    xacro_file = os.path.join(description_share, 'urdf', 'mock_arm.urdf.xacro')
    srdf_file = os.path.join(moveit_share, 'config', 'robot_arm.srdf')
    kinematics_file = os.path.join(moveit_share, 'config', 'kinematics.yaml')
    joint_limits_file = os.path.join(moveit_share, 'config', 'joint_limits.yaml')
    ompl_file = os.path.join(moveit_share, 'config', 'ompl_planning.yaml')
    controllers_file = os.path.join(moveit_share, 'config', 'moveit_controllers.yaml')

    robot_description = {
        'robot_description': xacro.process_file(xacro_file).toxml(),
        'robot_description_semantic': _load_text(srdf_file),
        'robot_description_kinematics': _load_yaml(kinematics_file),
        'robot_description_planning': _load_yaml(joint_limits_file),
    }

    move_group_parameters = [
        robot_description,
        _load_yaml(ompl_file),
        _load_yaml(controllers_file),
        {'use_sim_time': False},
    ]

    return LaunchDescription([
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[{'robot_description': robot_description['robot_description']}],
        ),
        Node(
            package='moveit_ros_move_group',
            executable='move_group',
            name='move_group',
            output='screen',
            parameters=move_group_parameters,
        ),
    ])
