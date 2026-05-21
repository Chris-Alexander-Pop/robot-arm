#include "bus_protocol/bus_frame_codec.h"

#include <cstring>

namespace robot_arm {
namespace bus {

namespace {

constexpr uint16_t kCrc16Poly = 0x1021U;
constexpr uint16_t kCrc16Init = 0xFFFFU;

}  // namespace

uint16_t BusFrameCodec::Crc16(const uint8_t* data, size_t length) {
  uint16_t crc = kCrc16Init;
  if (data == nullptr) {
    return crc;
  }

  for (size_t index = 0; index < length; ++index) {
    crc ^= static_cast<uint16_t>(data[index]) << 8U;
    for (int bit = 0; bit < 8; ++bit) {
      if ((crc & 0x8000U) != 0U) {
        crc = static_cast<uint16_t>((crc << 1U) ^ kCrc16Poly);
      } else {
        crc = static_cast<uint16_t>(crc << 1U);
      }
    }
  }
  return crc;
}

size_t BusFrameCodec::Encode(const BusFrame& frame, uint8_t* out, size_t capacity) const {
  if (out == nullptr || frame.payload_len > kMaxPayloadBytes) {
    return 0U;
  }

  const size_t frame_size = kFrameOverheadBytes + frame.payload_len;
  if (capacity < frame_size) {
    return 0U;
  }

  out[0] = kFrameHeader0;
  out[1] = kFrameHeader1;
  out[2] = frame.dst_addr;
  out[3] = frame.src_addr;
  out[4] = static_cast<uint8_t>(frame.command);
  out[5] = frame.payload_len;
  if (frame.payload_len > 0U) {
    std::memcpy(&out[6], frame.payload, frame.payload_len);
  }

  const uint16_t crc = Crc16(&out[2], 4U + frame.payload_len);
  out[6U + frame.payload_len] = static_cast<uint8_t>(crc & 0xFFU);
  out[7U + frame.payload_len] = static_cast<uint8_t>((crc >> 8U) & 0xFFU);
  return frame_size;
}

bool BusFrameCodec::Decode(const uint8_t* buffer, size_t length, BusFrame* out) const {
  if (buffer == nullptr || out == nullptr || length < kFrameOverheadBytes) {
    return false;
  }

  if (buffer[0] != kFrameHeader0 || buffer[1] != kFrameHeader1) {
    return false;
  }

  const uint8_t payload_len = buffer[5];
  const size_t expected_len = kFrameOverheadBytes + payload_len;
  if (length != expected_len || payload_len > kMaxPayloadBytes) {
    return false;
  }

  const uint16_t expected_crc = static_cast<uint16_t>(buffer[6U + payload_len]) |
                                (static_cast<uint16_t>(buffer[7U + payload_len]) << 8U);
  const uint16_t actual_crc = Crc16(&buffer[2], 4U + payload_len);
  if (expected_crc != actual_crc) {
    return false;
  }

  out->dst_addr = buffer[2];
  out->src_addr = buffer[3];
  out->command = static_cast<BusCommand>(buffer[4]);
  out->payload_len = payload_len;
  if (payload_len > 0U) {
    std::memcpy(out->payload, &buffer[6], payload_len);
  }
  return true;
}

}  // namespace bus
}  // namespace robot_arm
