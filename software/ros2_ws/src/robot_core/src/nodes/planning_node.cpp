#include <rclcpp/rclcpp.hpp>

namespace robot_core {

class PlanningNode : public rclcpp::Node {
 public:
  PlanningNode() : Node("planning_node") {
    RCLCPP_INFO(get_logger(), "Planning node initialized");
    // TODO: accept Cartesian goals and publish planned joint trajectories.
  }
};

}  // namespace robot_core

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<robot_core::PlanningNode>());
  rclcpp::shutdown();
  return 0;
}
