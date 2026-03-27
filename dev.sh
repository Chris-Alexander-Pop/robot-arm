#!/bin/bash

set -euo pipefail

COMPOSE_CMD="docker compose"

echo "=== Robot Arm Dev Helper ==="

if ! command -v docker &> /dev/null; then
    echo "Error: docker is not installed or not in PATH."
    exit 1
fi

if ! docker compose version &> /dev/null; then
    echo "Warning: docker compose plugin not detected. Checking docker-compose..."
    if command -v docker-compose &> /dev/null; then
        COMPOSE_CMD="docker-compose"
    else
        echo "Error: Neither 'docker compose' plugin nor 'docker-compose' is available."
        exit 1
    fi
fi

if [ ! -d "software" ]; then
    echo "Error: Please run this script from the root of the robot-arm project directory."
    exit 1
fi

COMMAND=${1:-}

case "$COMMAND" in
    up)
        echo "Starting ROS 2 Container in the background..."
        cd software && $COMPOSE_CMD up -d
        ;;
    down)
        echo "Stopping ROS 2 Container..."
        cd software && $COMPOSE_CMD down
        ;;
    shell)
        echo "Dropping into ROS 2 Container shell..."
        cd software && $COMPOSE_CMD exec ros2 bash
        ;;
    build)
        echo "Building ROS 2 Workspace (colcon build)..."
        cd software && $COMPOSE_CMD exec -w /ros2_ws ros2 bash -c "source /opt/ros/humble/setup.bash && colcon build"
        ;;
    launch)
        echo "Launching RViz Visualization..."
        cd software && $COMPOSE_CMD exec -w /ros2_ws ros2 bash -c "\
            source /opt/ros/humble/setup.bash && \
            colcon build && \
            source install/setup.bash && \
            ros2 launch robot_description display.launch.py"
        ;;
    moveit-sim)
        echo "Launching MoveIt in simulation mode..."
        cd software && $COMPOSE_CMD exec -w /ros2_ws ros2 bash -c "\
            source /opt/ros/humble/setup.bash && \
            colcon build --packages-select robot_description robot_core robot_arm_moveit && \
            source install/setup.bash && \
            ros2 launch robot_arm_moveit sim.launch.py"
        ;;
    moveit-real)
        echo "Launching MoveIt in real-hardware mode..."
        cd software && $COMPOSE_CMD exec -w /ros2_ws ros2 bash -c "\
            source /opt/ros/humble/setup.bash && \
            colcon build --packages-select robot_description robot_core robot_arm_moveit && \
            source install/setup.bash && \
            ros2 launch robot_arm_moveit real.launch.py"
        ;;
    moveit-test)
        echo "Running ROS behavioral tests..."
        cd software && $COMPOSE_CMD exec -w /ros2_ws ros2 bash -c "\
            source /opt/ros/humble/setup.bash && \
            colcon build --packages-select robot_description robot_core robot_arm_moveit && \
            source install/setup.bash && \
            python3 -m pytest \
                /ros2_ws/src/robot_core/test/test_hardware_bridge_graph.py \
                /ros2_ws/src/robot_arm_moveit/test/test_moveit_config.py"
        ;;
    behavior-test)
        "$0" moveit-test
        ;;
    *)
        echo "Usage: ./dev.sh [COMMAND]"
        echo ""
        echo "Commands:"
        echo "  up            : Start the ROS 2 container in the background"
        echo "  down          : Stop the ROS 2 container"
        echo "  shell         : Open an interactive bash shell inside the container"
        echo "  build         : Run 'colcon build' inside the container"
        echo "  launch        : Build the workspace and instantly launch the RViz visualization"
        echo "  moveit-sim    : Build the workspace and launch MoveIt + RViz in simulation mode"
        echo "  moveit-real   : Build the workspace and launch MoveIt + RViz in real-hardware mode"
        echo "  moveit-test   : Run the ROS behavioral tests inside the container"
        echo "  behavior-test : Alias for moveit-test"
        exit 1
        ;;
esac
