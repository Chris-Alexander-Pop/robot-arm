#include <cstring>

#include "bus_protocol/bus_commands.h"
#include "bus_protocol/bus_frame_codec.h"
#include "test_harness.h"

namespace {

using robot_arm::bus::BusCommand;
using robot_arm::bus::BusFrame;
using robot_arm::bus::BusFrameCodec;
using robot_arm::bus::JointTargetPayload;
using robot_arm::bus::kAddrMaster;
using robot_arm::bus::kMaxFrameBytes;

void test_round_trip_encode_decode(TestContext& test) {
  BusFrameCodec codec;

  BusFrame frame;
  frame.dst_addr = 3U;
  frame.src_addr = kAddrMaster;
  frame.command = BusCommand::kSetJointTarget;
  JointTargetPayload payload{45.0f, 10.0f};
  frame.payload_len = static_cast<uint8_t>(sizeof(payload));
  std::memcpy(frame.payload, &payload, sizeof(payload));

  uint8_t buffer[kMaxFrameBytes] = {};
  const size_t encoded = codec.Encode(frame, buffer, kMaxFrameBytes);
  test.Check(encoded > 0U, "bus frame should encode");

  BusFrame decoded;
  test.Check(codec.Decode(buffer, encoded, &decoded), "bus frame should decode");
  test.Check(decoded.dst_addr == 3U, "dst addr");
  test.Check(decoded.command == BusCommand::kSetJointTarget, "command id");
  test.Check(decoded.payload_len == sizeof(payload), "payload len");

  JointTargetPayload round_trip;
  std::memcpy(&round_trip, decoded.payload, sizeof(round_trip));
  test.CheckFloatEq(45.0f, round_trip.position_deg, "position");
  test.CheckFloatEq(10.0f, round_trip.velocity_deg_s, "velocity");
}

void test_rejects_corrupted_crc(TestContext& test) {
  BusFrameCodec codec;
  BusFrame frame;
  frame.dst_addr = 1U;
  frame.src_addr = kAddrMaster;
  frame.command = BusCommand::kHeartbeat;

  uint8_t buffer[kMaxFrameBytes] = {};
  const size_t encoded = codec.Encode(frame, buffer, kMaxFrameBytes);
  test.Check(encoded > 0U, "heartbeat should encode");

  buffer[encoded - 1U] ^= 0xFFU;
  BusFrame decoded;
  test.Check(!codec.Decode(buffer, encoded, &decoded), "corrupted crc should fail decode");
}

}  // namespace

void run_bus_frame_codec_tests(TestContext& test) {
  test_round_trip_encode_decode(test);
  test_rejects_corrupted_crc(test);
}
