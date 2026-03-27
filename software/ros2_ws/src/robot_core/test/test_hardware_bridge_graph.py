import math
import os
import pty
import signal
import struct
import subprocess
import time
from pathlib import Path

import rclpy
from ament_index_python.packages import get_package_prefix
from builtin_interfaces.msg import Duration
from control_msgs.action import FollowJointTrajectory
from rclpy.action import ActionClient
from sensor_msgs.msg import JointState
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


def _checksum(data):
    value = 0
    for byte in data:
        value ^= byte
    return value


def _build_joint_state_frame(position_deg, velocity_deg_s):
    frame = bytearray(52)
    frame[0] = 0xAA
    frame[1] = 0x55
    frame[2] = 0x11
    struct.pack_into('<6f', frame, 3, *position_deg)
    struct.pack_into('<6f', frame, 27, *velocity_deg_s)
    frame[-1] = _checksum(frame[2:-1])
    return bytes(frame)


def _read_exact(file_descriptor, length, timeout_seconds):
    deadline = time.monotonic() + timeout_seconds
    data = bytearray()
    while len(data) < length and time.monotonic() < deadline:
        try:
            chunk = os.read(file_descriptor, length - len(data))
            if chunk:
                data.extend(chunk)
                continue
        except BlockingIOError:
            pass
        time.sleep(0.02)
    return bytes(data)


def _wait_for_message(node, message_box, timeout_seconds):
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        if message_box['message'] is not None:
            return True
        rclpy.spin_once(node, timeout_sec=0.1)
    return message_box['message'] is not None


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


def test_bridge_aborts_goal_when_serial_port_is_unavailable():
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
    node = rclpy.create_node('hardware_bridge_fault_probe')
    try:
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
        goal_handle = send_goal_future.result()
        assert goal_handle is not None and goal_handle.accepted, 'goal should be accepted before the serial failure is detected'

        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(node, result_future, timeout_sec=5.0)
        assert result_future.done(), 'hardware bridge never returned a result after serial failure'

        result = result_future.result()
        assert result is not None
        assert result.status == 6, 'hardware bridge should abort the goal when the serial port is unavailable'
    finally:
        node.destroy_node()
        rclpy.shutdown()

        if bridge_process.poll() is None:
            bridge_process.send_signal(signal.SIGINT)
            try:
                bridge_process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                bridge_process.kill()


def test_bridge_rejects_empty_or_mismatched_trajectory_goals():
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
    node = rclpy.create_node('hardware_bridge_rejection_probe')
    try:
        action_client = ActionClient(
            node,
            FollowJointTrajectory,
            'arm_controller/follow_joint_trajectory',
        )

        assert action_client.wait_for_server(timeout_sec=5.0), (
            'hardware bridge did not advertise arm_controller/follow_joint_trajectory'
        )

        empty_goal = FollowJointTrajectory.Goal()
        empty_goal.trajectory = JointTrajectory()
        empty_goal.trajectory.joint_names = [
            'joint1',
            'joint2',
            'joint3',
            'joint4',
            'joint5',
            'joint6',
        ]

        send_goal_future = action_client.send_goal_async(empty_goal)
        rclpy.spin_until_future_complete(node, send_goal_future, timeout_sec=5.0)
        goal_handle = send_goal_future.result()
        assert goal_handle is not None and not goal_handle.accepted, 'empty trajectory should be rejected'

        wrong_joint_count_goal = FollowJointTrajectory.Goal()
        wrong_joint_count_goal.trajectory = JointTrajectory()
        wrong_joint_count_goal.trajectory.joint_names = ['joint1', 'joint2', 'joint3']
        point = JointTrajectoryPoint()
        point.positions = [0.0, 0.0, 0.0]
        point.time_from_start = Duration(sec=1, nanosec=0)
        wrong_joint_count_goal.trajectory.points = [point]

        send_goal_future = action_client.send_goal_async(wrong_joint_count_goal)
        rclpy.spin_until_future_complete(node, send_goal_future, timeout_sec=5.0)
        goal_handle = send_goal_future.result()
        assert goal_handle is not None and not goal_handle.accepted, 'mismatched joint trajectory should be rejected'
    finally:
        node.destroy_node()
        rclpy.shutdown()

        if bridge_process.poll() is None:
            bridge_process.send_signal(signal.SIGINT)
            try:
                bridge_process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                bridge_process.kill()


def test_bridge_writes_set_joints_and_publishes_joint_states():
    master_fd, slave_fd = pty.openpty()
    slave_port = os.ttyname(slave_fd)
    os.set_blocking(master_fd, False)

    package_prefix = Path(get_package_prefix('robot_core'))
    executable = package_prefix / 'lib' / 'robot_core' / 'hardware_bridge_node'

    bridge_process = subprocess.Popen(
        [
            str(executable),
            '--ros-args',
            '-p',
            f'serial_port:={slave_port}',
            '-p',
            'baud_rate:=115200',
            '-p',
            'poll_period_ms:=20',
            '-p',
            'heartbeat_period_ms:=5000',
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        env=os.environ.copy(),
        text=True,
    )

    rclpy.init()
    node = rclpy.create_node('hardware_bridge_round_trip_probe')
    received_joint_state = {'message': None}

    def on_joint_state(message):
        received_joint_state['message'] = message

    subscription = node.create_subscription(JointState, '/joint_states', on_joint_state, 10)

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
        point.positions = [0.0, 0.5, -0.25, 1.0, -1.5, 0.75]
        point.time_from_start = Duration(sec=1, nanosec=0)
        goal.trajectory.points = [point]

        send_goal_future = action_client.send_goal_async(goal)
        rclpy.spin_until_future_complete(node, send_goal_future, timeout_sec=5.0)
        assert send_goal_future.done(), 'trajectory goal was not sent to the bridge'

        goal_handle = send_goal_future.result()
        assert goal_handle is not None and goal_handle.accepted, (
            'hardware bridge rejected the follow_joint_trajectory goal'
        )

        command_frame = _read_exact(master_fd, 28, 2.0)
        assert len(command_frame) == 28, 'STM32 command frame was not written to the serial port'
        assert command_frame[0] == 0xAA and command_frame[1] == 0x55
        assert command_frame[2] == 0x10
        decoded_positions_deg = struct.unpack_from('<6f', command_frame, 3)
        expected_positions_deg = [value * 180.0 / math.pi for value in point.positions]
        for actual_value, expected_value in zip(decoded_positions_deg, expected_positions_deg):
            assert abs(actual_value - expected_value) < 1e-4
        assert command_frame[-1] == _checksum(command_frame[2:-1])

        assert _wait_for_message(node, received_joint_state, 5.0), (
            'hardware bridge did not publish joint states after accepting a trajectory goal'
        )

        joint_state_message = received_joint_state['message']
        assert joint_state_message is not None

        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(node, result_future, timeout_sec=5.0)
        assert result_future.done(), 'hardware bridge never returned a trajectory result'

        result = result_future.result()
        assert result is not None
        assert result.status in (4, 5, 6), 'hardware bridge returned an unexpected action result status'
    finally:
        node.destroy_subscription(subscription)
        node.destroy_node()
        rclpy.shutdown()

        os.close(master_fd)
        os.close(slave_fd)

        if bridge_process.poll() is None:
            bridge_process.send_signal(signal.SIGINT)
            try:
                bridge_process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                bridge_process.kill()


def test_bridge_sends_heartbeat_frames_over_serial():
    master_fd, slave_fd = pty.openpty()
    slave_port = os.ttyname(slave_fd)
    os.set_blocking(master_fd, False)

    package_prefix = Path(get_package_prefix('robot_core'))
    executable = package_prefix / 'lib' / 'robot_core' / 'hardware_bridge_node'

    bridge_process = subprocess.Popen(
        [
            str(executable),
            '--ros-args',
            '-p',
            f'serial_port:={slave_port}',
            '-p',
            'baud_rate:=115200',
            '-p',
            'poll_period_ms:=50',
            '-p',
            'heartbeat_period_ms:=200',
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        env=os.environ.copy(),
        text=True,
    )

    rclpy.init()
    node = rclpy.create_node('hardware_bridge_heartbeat_probe')

    try:
        assert _read_exact(master_fd, 4, 3.0) == bytes([0xAA, 0x55, 0x12, 0x12]), (
            'hardware bridge did not emit the expected heartbeat frame'
        )
    finally:
        node.destroy_node()
        rclpy.shutdown()

        os.close(master_fd)
        os.close(slave_fd)

        if bridge_process.poll() is None:
            bridge_process.send_signal(signal.SIGINT)
            try:
                bridge_process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                bridge_process.kill()