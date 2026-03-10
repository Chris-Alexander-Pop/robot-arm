# Use the official ROS 2 Humble desktop image as a base
FROM docker.io/osrf/ros:humble-desktop

# Define the workspace directory
ENV ROS_WS=/ros2_ws
WORKDIR $ROS_WS

# Install useful development tools and dependencies
RUN apt-get update && apt-get install -y \
    python3-pip \
    python3-colcon-common-extensions \
    ros-humble-joint-state-publisher \
    ros-humble-joint-state-publisher-gui \
    ros-humble-xacro \
    ros-humble-rviz2 \
    ros-humble-ros2-control \
    ros-humble-ros2-controllers \
    nano \
    git \
 && rm -rf /var/lib/apt/lists/*

# Add source command to bashrc so ROS 2 is always active when you enter the container
RUN echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
RUN echo "if [ -f ${ROS_WS}/install/setup.bash ]; then source ${ROS_WS}/install/setup.bash; fi" >> ~/.bashrc

CMD ["bash"]
