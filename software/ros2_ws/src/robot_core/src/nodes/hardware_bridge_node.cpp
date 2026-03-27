#include <algorithm>
#include <chrono>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <trajectory_msgs/msg/joint_trajectory.hpp>

#include "robot_core/adapters/serial_hardware_interface.hpp"

namespace robot_core {

class HardwareBridgeNode : public rclcpp::Node {
 public:
  HardwareBridgeNode()
  : Node("hardware_bridge_node"), hardware_interface_(std::make_unique<SerialHardwareInterface>()) {
    declare_parameter<std::string>("serial_port", "/dev/ttyUSB0");
    declare_parameter<int>("baud_rate", 115200);
    declare_parameter<int>("poll_period_ms", 20);

    hardware_interface_->SetPort(get_parameter("serial_port").as_string());
    hardware_interface_->SetBaudRate(get_parameter("baud_rate").as_int());

    joint_state_publisher_ = create_publisher<sensor_msgs::msg::JointState>("joint_states", 10);
    trajectory_subscription_ = create_subscription<trajectory_msgs::msg::JointTrajectory>(
      "command_trajectory",
      10,
      [this](const trajectory_msgs::msg::JointTrajectory::SharedPtr message) {
        if (message == nullptr || message->points.empty()) {
          return;
        }

        MotionGoal goal{};
        const auto& point = message->points.back();
        const std::size_t count = std::min<std::size_t>(goal.target_joint_rad.size(), point.positions.size());
        for (std::size_t index = 0U; index < count; ++index) {
          goal.target_joint_rad[index] = point.positions[index];
        }

        if (!hardware_interface_->SendGoal(goal)) {
          RCLCPP_WARN(get_logger(), "Failed to forward trajectory to the STM32");
        }
      });

    poll_timer_ = create_wall_timer(std::chrono::milliseconds(get_parameter("poll_period_ms").as_int()), [this]() {
      if (!hardware_interface_->IsConnected() && !hardware_interface_->Connect()) {
        RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 5000, "Waiting for STM32 serial bridge at %s",
          get_parameter("serial_port").as_string().c_str());
        return;
      }

      const RobotState state = hardware_interface_->ReadState();
      sensor_msgs::msg::JointState joint_state_message;
      joint_state_message.header.stamp = now();
      joint_state_message.name = {"joint1", "joint2", "joint3", "joint4", "joint5", "joint6"};
      joint_state_message.position.assign(state.joint_position_rad.begin(), state.joint_position_rad.end());
      joint_state_message.velocity.assign(state.joint_velocity_rad_s.begin(), state.joint_velocity_rad_s.end());
      joint_state_publisher_->publish(joint_state_message);
    });

    RCLCPP_INFO(get_logger(), "Hardware bridge node initialized");
    if (!hardware_interface_->Connect()) {
      RCLCPP_WARN(get_logger(), "STM32 serial bridge is not available yet");
    }
  }

 private:
  std::unique_ptr<SerialHardwareInterface> hardware_interface_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_publisher_;
  rclcpp::Subscription<trajectory_msgs::msg::JointTrajectory>::SharedPtr trajectory_subscription_;
  rclcpp::TimerBase::SharedPtr poll_timer_;
};

}  // namespace robot_core

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<robot_core::HardwareBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
