#include "robot_core/adapters/hardware_interface.hpp"

namespace robot_core {

class SerialHardwareInterface final : public HardwareInterface {
 public:
  bool Connect() override {
    // TODO: open and configure serial transport to MCU.
    return false;
  }

  bool SendGoal(const MotionGoal& goal) override {
    (void)goal;
    // TODO: serialize and send packet to firmware.
    return false;
  }

  RobotState ReadState() override {
    // TODO: decode incoming state packet.
    return RobotState{};
  }
};

}  // namespace robot_core
