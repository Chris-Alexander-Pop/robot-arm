#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "robot_core/adapters/hardware_interface.hpp"

namespace robot_core {

class SerialHardwareInterface final : public HardwareInterface {
 public:
  SerialHardwareInterface();
  explicit SerialHardwareInterface(std::string port_name, int baud_rate = 115200);
  ~SerialHardwareInterface() override;

  void SetPort(std::string port_name);
  void SetBaudRate(int baud_rate);

  bool Connect() override;
  bool SendGoal(const MotionGoal& goal) override;
  RobotState ReadState() override;
  bool SendHeartbeat();
  void Close();

  bool IsConnected() const;

 private:
  bool ConfigurePort();
  bool DrainReadBuffer();
  void AppendReadBytes(const uint8_t* data, std::size_t length);
  void ConsumeReadBytes(std::size_t length);

  int file_descriptor_;
  std::string port_name_;
  int baud_rate_;
  RobotState last_state_{};
  std::array<uint8_t, 128U> read_buffer_{};
  std::size_t read_buffer_size_;
};

}  // namespace robot_core