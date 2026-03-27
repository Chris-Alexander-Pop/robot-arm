#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <control_msgs/action/follow_joint_trajectory.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <sensor_msgs/msg/joint_state.hpp>

#include "robot_core/adapters/serial_hardware_interface.hpp"

namespace robot_core {

class HardwareBridgeNode : public rclcpp::Node {
 public:
  using FollowJointTrajectory = control_msgs::action::FollowJointTrajectory;
  using GoalHandleFollowJointTrajectory = rclcpp_action::ServerGoalHandle<FollowJointTrajectory>;

  HardwareBridgeNode()
  : Node("hardware_bridge_node"), hardware_interface_(std::make_unique<SerialHardwareInterface>()) {
    declare_parameter<std::string>("serial_port", "/dev/ttyUSB0");
    declare_parameter<int>("baud_rate", 115200);
    declare_parameter<int>("poll_period_ms", 20);
    declare_parameter<int>("heartbeat_period_ms", 1000);
    declare_parameter<std::vector<std::string>>("joint_names", {"joint1", "joint2", "joint3", "joint4", "joint5", "joint6"});

    hardware_interface_->SetPort(get_parameter("serial_port").as_string());
    hardware_interface_->SetBaudRate(get_parameter("baud_rate").as_int());

    joint_state_publisher_ = create_publisher<sensor_msgs::msg::JointState>("joint_states", 10);

    trajectory_action_server_ = rclcpp_action::create_server<FollowJointTrajectory>(
      this,
      "arm_controller/follow_joint_trajectory",
      [this](const rclcpp_action::GoalUUID&, std::shared_ptr<const FollowJointTrajectory::Goal> goal) {
        if (goal == nullptr || goal->trajectory.points.empty()) {
          return rclcpp_action::GoalResponse::REJECT;
        }

        if (!goal->trajectory.joint_names.empty() && goal->trajectory.joint_names.size() != kJointCount) {
          RCLCPP_WARN(get_logger(), "Rejecting trajectory with %zu joints", goal->trajectory.joint_names.size());
          return rclcpp_action::GoalResponse::REJECT;
        }

        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
      },
      [this](const std::shared_ptr<GoalHandleFollowJointTrajectory>) {
        return rclcpp_action::CancelResponse::ACCEPT;
      },
      [this](const std::shared_ptr<GoalHandleFollowJointTrajectory> goal_handle) {
        std::thread([this, goal_handle]() { ExecuteTrajectory(goal_handle); }).detach();
      });

    poll_timer_ = create_wall_timer(std::chrono::milliseconds(get_parameter("poll_period_ms").as_int()), [this]() {
      PublishJointState();
    });

    heartbeat_timer_ = create_wall_timer(std::chrono::milliseconds(get_parameter("heartbeat_period_ms").as_int()), [this]() {
      std::lock_guard<std::mutex> lock(hardware_mutex_);
      if (!hardware_interface_->SendHeartbeat()) {
        RCLCPP_WARN(get_logger(), "Heartbeat to STM32 failed; closing serial link");
        hardware_interface_->Close();
      }
    });

    RCLCPP_INFO(get_logger(), "Hardware bridge node initialized");
  }

 private:
  void ExecuteTrajectory(const std::shared_ptr<GoalHandleFollowJointTrajectory>& goal_handle) {
    const auto goal = goal_handle->get_goal();
    auto result = std::make_shared<FollowJointTrajectory::Result>();

    if (goal == nullptr) {
      result->error_code = FollowJointTrajectory::Result::INVALID_GOAL;
      goal_handle->abort(result);
      return;
    }

    if (!EnsureConnection()) {
      result->error_code = FollowJointTrajectory::Result::PATH_TOLERANCE_VIOLATED;
      result->error_string = "STM32 serial link unavailable";
      goal_handle->abort(result);
      return;
    }

    const rclcpp::Time trajectory_start = now();

    for (const auto& point : goal->trajectory.points) {
      if (goal_handle->is_canceling()) {
        result->error_code = FollowJointTrajectory::Result::INVALID_GOAL;
        goal_handle->canceled(result);
        return;
      }

      MotionGoal motion_goal{};
      const std::size_t count = std::min<std::size_t>(motion_goal.target_joint_rad.size(), point.positions.size());
      for (std::size_t index = 0U; index < count; ++index) {
        motion_goal.target_joint_rad[index] = point.positions[index];
      }

      if (!SendGoal(motion_goal)) {
        result->error_code = FollowJointTrajectory::Result::PATH_TOLERANCE_VIOLATED;
        result->error_string = "Failed to send trajectory point to STM32";
        goal_handle->abort(result);
        return;
      }

      const rclcpp::Duration scheduled_duration(point.time_from_start);
      const rclcpp::Duration wait_duration = scheduled_duration - (now() - trajectory_start);
      if (wait_duration.nanoseconds() > 0) {
        rclcpp::sleep_for(std::chrono::nanoseconds(wait_duration.nanoseconds()));
      }
    }

    PublishJointState();
    result->error_code = FollowJointTrajectory::Result::SUCCESSFUL;
    goal_handle->succeed(result);
  }

  bool EnsureConnection() {
    std::lock_guard<std::mutex> lock(hardware_mutex_);
    if (hardware_interface_->IsConnected()) {
      return true;
    }

    return hardware_interface_->Connect();
  }

  bool SendGoal(const MotionGoal& goal) {
    std::lock_guard<std::mutex> lock(hardware_mutex_);
    if (!hardware_interface_->Connect()) {
      return false;
    }

    if (!hardware_interface_->SendGoal(goal)) {
      hardware_interface_->Close();
      return false;
    }

    return true;
  }

  void PublishJointState() {
    if (!EnsureConnection()) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 5000, "Waiting for STM32 serial bridge at %s",
        get_parameter("serial_port").as_string().c_str());
      return;
    }

    RobotState state{};
    {
      std::lock_guard<std::mutex> lock(hardware_mutex_);
      state = hardware_interface_->ReadState();
    }

    sensor_msgs::msg::JointState joint_state_message;
    joint_state_message.header.stamp = now();
    joint_state_message.name = get_parameter("joint_names").as_string_array();
    joint_state_message.position.assign(state.joint_position_rad.begin(), state.joint_position_rad.end());
    joint_state_message.velocity.assign(state.joint_velocity_rad_s.begin(), state.joint_velocity_rad_s.end());
    joint_state_publisher_->publish(joint_state_message);
  }

  std::unique_ptr<SerialHardwareInterface> hardware_interface_;
  std::mutex hardware_mutex_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_publisher_;
  rclcpp_action::Server<FollowJointTrajectory>::SharedPtr trajectory_action_server_;
  rclcpp::TimerBase::SharedPtr poll_timer_;
  rclcpp::TimerBase::SharedPtr heartbeat_timer_;
};

}  // namespace robot_core

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<robot_core::HardwareBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
