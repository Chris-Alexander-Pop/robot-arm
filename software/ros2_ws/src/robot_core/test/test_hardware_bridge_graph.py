import os
import signal
import subprocess
import time
from pathlib import Path

import rclpy
from ament_index_python.packages import get_package_prefix
from builtin_interfaces.msg import Duration
from control_msgs.action import FollowJointTrajectory
from rclpy.action import ActionClient
from trajectory_msgs.msg import JointTrajectory, JointTrajectoryPoint


def _names_from_graph(entries):
    return {name if name.startswith('/') else f'/{name}' for name, _ in entries}


def _wait_for(condition, timeout_seconds):
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        if condition():
            return True
        time.sleep(0.2)
    return False


def test_bridge_exposes_joint_states_and_accepts_trajectory_goal():
    package_prefix = Path(get_package_prefix('robot_core'))
    executable = package_prefix / 'lib' / 'robot_core' / 'hardware_bridge_node'

    bridge_process = subprocess.Popen(
        [
            str(executable),
            '--ros-args',
            '-p',
            'serial_port:=/dev/ttyDoesNotExist',
            '-p',
            'baud_rate:=115200',
            '-p',
            'poll_period_ms:=20',
            '-p',
            'heartbeat_period_ms:=1000',
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        env=os.environ.copy(),
        text=True,
    )

    rclpy.init()
    node = rclpy.create_node('hardware_bridge_graph_probe')
    try:
        assert _wait_for(
            lambda: '/joint_states' in _names_from_graph(node.get_topic_names_and_types()),
            5.0,
        ), 'hardware bridge did not advertise /joint_states'

        action_client = ActionClient(
            node,
            FollowJointTrajectory,
            'arm_controller/follow_joint_trajectory',
        )

        assert action_client.wait_for_server(timeout_sec=5.0), (
            'hardware bridge did not advertise arm_controller/follow_joint_trajectory'
        )

        goal = FollowJointTrajectory.Goal()
        goal.trajectory = JointTrajectory()
        goal.trajectory.joint_names = [
            'joint1',
            'joint2',
            'joint3',
            'joint4',
            'joint5',
            'joint6',
        ]

        point = JointTrajectoryPoint()
        point.positions = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
        point.time_from_start = Duration(sec=1, nanosec=0)
        goal.trajectory.points = [point]

        send_goal_future = action_client.send_goal_async(goal)
        rclpy.spin_until_future_complete(node, send_goal_future, timeout_sec=5.0)
        assert send_goal_future.done(), 'trajectory goal was not sent to the bridge'

        goal_handle = send_goal_future.result()
        assert goal_handle is not None and goal_handle.accepted, (
            'hardware bridge rejected the follow_joint_trajectory goal'
        )

        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(node, result_future, timeout_sec=5.0)
        assert result_future.done(), 'hardware bridge never returned a trajectory result'

        result = result_future.result()
        assert result is not None
        assert result.status in (
            4,
            5,
            6,
        ), 'hardware bridge returned an unexpected action result status'
    finally:
        node.destroy_node()
        rclpy.shutdown()

        if bridge_process.poll() is None:
            bridge_process.send_signal(signal.SIGINT)
            try:
                bridge_process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                bridge_process.kill()