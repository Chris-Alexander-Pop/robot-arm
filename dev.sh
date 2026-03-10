#!/bin/bash

# Configuration
COMPOSE_CMD="podman-compose" # Default to podman, change to docker-compose if needed
ROS_WS_DIR="$(pwd)/software/ros2_ws"

echo "=== Robot Arm Dev Helper ==="

# Check if podman-compose is installed
if ! command -v $COMPOSE_CMD &> /dev/null; then
    echo "Error: $COMPOSE_CMD not found. Checking for docker-compose..."
    if command -v docker-compose &> /dev/null; then
        COMPOSE_CMD="docker-compose"
    else
        echo "Error: Neither podman-compose nor docker-compose found."
        exit 1
    fi
fi

# Ensure we are in the root of the project by checking if software/ exists
if [ ! -d "software" ]; then
    echo "Error: Please run this script from the root of the robot-arm project directory."
    exit 1
fi

COMMAND=$1

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
        # First ensure the workspace is built
        cd software && $COMPOSE_CMD exec -w /ros2_ws ros2 bash -c "\
            source /opt/ros/humble/setup.bash && \
            colcon build && \
            source install/setup.bash && \
            ros2 launch robot_description display.launch.py"
        ;;
    *)
        echo "Usage: ./dev.sh [COMMAND]"
        echo ""
        echo "Commands:"
        echo "  up      : Start the ROS 2 container in the background"
        echo "  down    : Stop the ROS 2 container"
        echo "  shell   : Open an interactive bash shell inside the container"
        echo "  build   : Run 'colcon build' inside the container"
        echo "  launch  : Build the workspace and instantly launch the RViz visualization"
        ;;
esac
