#include "robot_core/adapters/serial_hardware_interface.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <utility>
#include <termios.h>
#include <unistd.h>

namespace robot_core {

namespace {

constexpr uint8_t kFrameHeader0 = 0xAAU;
constexpr uint8_t kFrameHeader1 = 0x55U;
constexpr uint8_t kSetJointCommandId = 0x10U;
constexpr uint8_t kHeartbeatId = 0x12U;
constexpr uint8_t kJointStateId = 0x11U;
constexpr std::size_t kJointCountBytes = sizeof(float) * kJointCount;
constexpr std::size_t kCommandFrameSize = 2U + 1U + kJointCountBytes + 1U;
constexpr std::size_t kHeartbeatFrameSize = 2U + 1U + 1U;
constexpr std::size_t kStatePayloadBytes = 2U * kJointCountBytes;
constexpr std::size_t kStateFrameSize = 2U + 1U + kStatePayloadBytes + 1U;
constexpr double kRadiansPerDegree = 3.14159265358979323846 / 180.0;
constexpr double kDegreesPerRadian = 180.0 / 3.14159265358979323846;

uint8_t ComputeChecksum(const uint8_t* data, std::size_t length) {
  uint8_t checksum = 0U;
  for (std::size_t index = 0U; index < length; ++index) {
    checksum ^= data[index];
  }
  return checksum;
}

speed_t ToBaudRate(int baud_rate) {
  switch (baud_rate) {
    case 115200:
      return B115200;
    case 230400:
      return B230400;
    case 460800:
      return B460800;
    case 921600:
      return B921600;
    default:
      return B0;
  }
}

bool WriteAll(int file_descriptor, const uint8_t* data, std::size_t length) {
  std::size_t offset = 0U;
  while (offset < length) {
    const ssize_t written = ::write(file_descriptor, data + offset, length - offset);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    offset += static_cast<std::size_t>(written);
  }
  return true;
}

RobotState DecodeStateFrame(const uint8_t* frame) {
  RobotState state{};
  std::array<float, kJointCount> positions_deg{};
  std::array<float, kJointCount> velocities_deg_s{};
  std::memcpy(positions_deg.data(), &frame[3], kJointCountBytes);
  std::memcpy(velocities_deg_s.data(), &frame[3U + kJointCountBytes], kJointCountBytes);

  for (std::size_t joint = 0U; joint < kJointCount; ++joint) {
    state.joint_position_rad[joint] = static_cast<double>(positions_deg[joint]) * kRadiansPerDegree;
    state.joint_velocity_rad_s[joint] = static_cast<double>(velocities_deg_s[joint]) * kRadiansPerDegree;
  }

  return state;
}

}  // namespace

SerialHardwareInterface::SerialHardwareInterface()
: file_descriptor_(-1), port_name_("/dev/ttyUSB0"), baud_rate_(115200), read_buffer_size_(0U) {}

SerialHardwareInterface::SerialHardwareInterface(std::string port_name, int baud_rate)
: file_descriptor_(-1), port_name_(std::move(port_name)), baud_rate_(baud_rate), read_buffer_size_(0U) {}

SerialHardwareInterface::~SerialHardwareInterface() {
  if (file_descriptor_ >= 0) {
    ::close(file_descriptor_);
  }
}

void SerialHardwareInterface::SetPort(std::string port_name) {
  port_name_ = std::move(port_name);
}

void SerialHardwareInterface::SetBaudRate(int baud_rate) {
  baud_rate_ = baud_rate;
}

bool SerialHardwareInterface::IsConnected() const {
  return file_descriptor_ >= 0;
}

bool SerialHardwareInterface::Connect() {
  if (file_descriptor_ >= 0) {
    return true;
  }

  file_descriptor_ = ::open(port_name_.c_str(), O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
  if (file_descriptor_ < 0) {
    return false;
  }

  if (!ConfigurePort()) {
    ::close(file_descriptor_);
    file_descriptor_ = -1;
    return false;
  }

  read_buffer_size_ = 0U;
  return true;
}

void SerialHardwareInterface::Close() {
  if (file_descriptor_ >= 0) {
    ::close(file_descriptor_);
    file_descriptor_ = -1;
  }
  read_buffer_size_ = 0U;
}

bool SerialHardwareInterface::SendGoal(const MotionGoal& goal) {
  if (!Connect()) {
    return false;
  }

  std::array<float, kJointCount> target_deg{};
  for (std::size_t joint = 0U; joint < kJointCount; ++joint) {
    target_deg[joint] = static_cast<float>(goal.target_joint_rad[joint] * kDegreesPerRadian);
  }

  std::array<uint8_t, kCommandFrameSize> frame{};
  frame[0] = kFrameHeader0;
  frame[1] = kFrameHeader1;
  frame[2] = kSetJointCommandId;
  std::memcpy(&frame[3], target_deg.data(), kJointCountBytes);
  frame[kCommandFrameSize - 1U] = ComputeChecksum(&frame[2], 1U + kJointCountBytes);

  const bool write_ok = WriteAll(file_descriptor_, frame.data(), frame.size());
  if (!write_ok) {
    Close();
  }
  return write_ok;
}

bool SerialHardwareInterface::SendHeartbeat() {
  if (!Connect()) {
    return false;
  }

  std::array<uint8_t, kHeartbeatFrameSize> frame{};
  frame[0] = kFrameHeader0;
  frame[1] = kFrameHeader1;
  frame[2] = kHeartbeatId;
  frame[3] = ComputeChecksum(&frame[2], 1U);

  const bool write_ok = WriteAll(file_descriptor_, frame.data(), frame.size());
  if (!write_ok) {
    Close();
  }
  return write_ok;
}

RobotState SerialHardwareInterface::ReadState() {
  if (!Connect()) {
    return last_state_;
  }

  if (!DrainReadBuffer()) {
    Close();
    return last_state_;
  }

  while (read_buffer_size_ >= kStateFrameSize) {
    if (read_buffer_[0] != kFrameHeader0 || read_buffer_[1] != kFrameHeader1) {
      ConsumeReadBytes(1U);
      continue;
    }

    if (read_buffer_[2] != kJointStateId) {
      ConsumeReadBytes(1U);
      continue;
    }

    if (ComputeChecksum(read_buffer_.data() + 2U, 1U + kStatePayloadBytes) == read_buffer_[kStateFrameSize - 1U]) {
      last_state_ = DecodeStateFrame(read_buffer_.data());
      ConsumeReadBytes(kStateFrameSize);
      return last_state_;
    }

    ConsumeReadBytes(1U);
  }

  return last_state_;
}

bool SerialHardwareInterface::ConfigurePort() {
  termios options{};
  if (tcgetattr(file_descriptor_, &options) != 0) {
    return false;
  }

  cfmakeraw(&options);

  const speed_t baud_rate = ToBaudRate(baud_rate_);
  if (baud_rate == B0) {
    return false;
  }

  if (cfsetispeed(&options, baud_rate) != 0 || cfsetospeed(&options, baud_rate) != 0) {
    return false;
  }

  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~CRTSCTS;
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  options.c_cc[VMIN] = 0;
  options.c_cc[VTIME] = 1;

  return tcsetattr(file_descriptor_, TCSANOW, &options) == 0;
}

bool SerialHardwareInterface::DrainReadBuffer() {
  std::array<uint8_t, 64U> chunk{};
  while (true) {
    const ssize_t bytes_read = ::read(file_descriptor_, chunk.data(), chunk.size());
    if (bytes_read < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        break;
      }
      return false;
    }

    if (bytes_read == 0) {
      break;
    }

    AppendReadBytes(chunk.data(), static_cast<std::size_t>(bytes_read));
  }

  return true;
}

void SerialHardwareInterface::AppendReadBytes(const uint8_t* data, std::size_t length) {
  if (data == nullptr || length == 0U || read_buffer_size_ >= read_buffer_.size()) {
    return;
  }

  const std::size_t available = read_buffer_.size() - read_buffer_size_;
  const std::size_t bytes_to_copy = std::min(available, length);
  std::memcpy(read_buffer_.data() + read_buffer_size_, data, bytes_to_copy);
  read_buffer_size_ += bytes_to_copy;
}

void SerialHardwareInterface::ConsumeReadBytes(std::size_t length) {
  if (length >= read_buffer_size_) {
    read_buffer_size_ = 0U;
    return;
  }

  std::memmove(read_buffer_.data(), read_buffer_.data() + length, read_buffer_size_ - length);
  read_buffer_size_ -= length;
}

}  // namespace robot_core
