// Hardware test: Serial protocol loopback (RPi <-> STM32 link)
//
// Required hardware:
//   - Nucleo-F401RE connected via ST-Link USB (for flashing and monitoring)
//   - Host computer (or Raspberry Pi) with a serial terminal or the robot_arm
//     host software, connected at 115200 baud on the same UART
//   - USB cable or UART adapter providing the host link
//
// What this test does:
//   - Waits for a valid JointCommand packet (0xAA 0x55 0x10 ... checksum)
//   - Decodes it and immediately echoes back a JointState packet with the
//     commanded positions reflected in position_deg
//   - Also accepts heartbeat packets (0xAA 0x55 0x12 checksum) and responds
//     with a heartbeat echo
//   - Prints decoded values to Serial so you can watch progress in monitor
//
// Success criteria:
//   - Host sends a JointCommand → STM32 decodes it and echoes JointState
//   - Reflected position values match what the host sent
//   - Heartbeat round-trip works without framing errors
//
// Flash and monitor:
//   cd firmware/scripts/hardware_tests && ./run_comms.sh

#ifdef HWTEST_COMMS

#include <Arduino.h>

#include "protocol/packet_codec.h"
#include "core/types.h"

static constexpr size_t kCommandFrameSize   = 28U;
static constexpr size_t kHeartbeatFrameSize = 4U;
static constexpr size_t kStateFrameSize     = 52U;

static robot_arm::PacketCodec codec;
static uint8_t rx_buf[kCommandFrameSize]{};
static size_t  rx_len = 0U;
static uint32_t packets_received = 0U;

static void send_state(const robot_arm::JointState& state) {
  uint8_t frame[kStateFrameSize]{};
  const size_t encoded = codec.EncodeJointState(state, frame, sizeof(frame));
  if (encoded > 0U) {
    Serial.write(frame, encoded);
  }
}

static void send_heartbeat() {
  uint8_t frame[kHeartbeatFrameSize]{};
  const size_t encoded = codec.EncodeHeartbeat(frame, sizeof(frame));
  if (encoded > 0U) {
    Serial.write(frame, encoded);
  }
}

void hwtest_comms_setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000UL) { delay(10); }

  Serial.println("========================================");
  Serial.println("HWTEST: Serial protocol loopback");
  Serial.println("========================================");
  Serial.println("Waiting for JointCommand or heartbeat packets...");
  Serial.println("Frame format: 0xAA 0x55 <id> [payload] <checksum>");
  Serial.println("  0x10 = JointCommand (28 bytes total)");
  Serial.println("  0x12 = Heartbeat    (4 bytes total)");
  Serial.println();
}

void hwtest_comms_loop() {
  while (Serial.available() > 0) {
    const int val = Serial.read();
    if (val < 0) break;

    const uint8_t byte = static_cast<uint8_t>(val);

    // Sync on frame header.
    if (rx_len == 0U && byte != 0xAAU) continue;

    rx_buf[rx_len++] = byte;

    // Validate two-byte header.
    if (rx_len == 2U && (rx_buf[0] != 0xAAU || rx_buf[1] != 0x55U)) {
      rx_len = 0U;
      continue;
    }

    // Reject unknown command IDs early.
    if (rx_len == 3U) {
      const uint8_t id = rx_buf[2];
      if (id != 0x10U && id != 0x12U) {
        rx_len = 0U;
      }
      continue;
    }

    // Handle heartbeat (4 bytes).
    if (rx_len == kHeartbeatFrameSize && rx_buf[2] == 0x12U) {
      if (codec.DecodeHeartbeat(rx_buf, kHeartbeatFrameSize)) {
        ++packets_received;
        Serial.print("[HB #");
        Serial.print(packets_received);
        Serial.println("] Heartbeat received -> echoing back");
        send_heartbeat();
      } else {
        Serial.println("[HB] Bad heartbeat checksum — discarding");
      }
      rx_len = 0U;
      continue;
    }

    // Handle JointCommand (28 bytes).
    if (rx_len == kCommandFrameSize) {
      robot_arm::JointCommand cmd{};
      if (codec.DecodeJointCommand(rx_buf, kCommandFrameSize, &cmd)) {
        ++packets_received;
        Serial.print("[CMD #");
        Serial.print(packets_received);
        Serial.print("] JointCommand decoded. Targets (deg): ");
        for (int j = 0; j < robot_arm::kJointCount; ++j) {
          Serial.print(cmd.target_position_deg[j], 2);
          if (j < robot_arm::kJointCount - 1) Serial.print(", ");
        }
        Serial.println();

        // Echo back a JointState with positions mirroring the command.
        robot_arm::JointState echo{};
        for (int j = 0; j < robot_arm::kJointCount; ++j) {
          echo.position_deg[j]  = cmd.target_position_deg[j];
          echo.velocity_deg_s[j] = 0.0F;
        }
        send_state(echo);
        Serial.println("  JointState echoed back.");
      } else {
        Serial.println("[CMD] Bad checksum or framing error — discarding");
      }
      rx_len = 0U;
    }
  }
}

#endif  // HWTEST_COMMS
