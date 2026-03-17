#include <rclcpp/rclcpp.hpp>

namespace robot_core {

class HardwareBridgeNode : public rclcpp::Node {
 public:
  HardwareBridgeNode() : Node("hardware_bridge_node") {
    RCLCPP_INFO(get_logger(), "Hardware bridge node initialized");
    // TODO: subscribe to trajectory/goal topics and forward to MCU adapter.
  }
};

}  // namespace robot_core

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<robot_core::HardwareBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
