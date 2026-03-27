import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():
    moveit_share = get_package_share_directory('robot_arm_moveit')
    description_share = get_package_share_directory('robot_description')

    return LaunchDescription([
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(moveit_share, 'launch', 'moveit.launch.py')
            )
        ),
        Node(
            package='robot_core',
            executable='hardware_bridge_node',
            name='hardware_bridge_node',
            output='screen',
            parameters=[{
                'serial_port': '/dev/ttyUSB0',
                'baud_rate': 115200,
                'poll_period_ms': 20,
                'heartbeat_period_ms': 1000,
            }],
        ),
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            output='screen',
            arguments=['-d', os.path.join(description_share, 'rviz', 'display.rviz')],
        ),
    ])
