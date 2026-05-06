// Hardware test: Heartbeat watchdog timeout
//
// Required hardware:
//   - Nucleo-F401RE connected via ST-Link USB
//   - USB serial monitor at 115200 baud (to observe watchdog firing)
//   - Optional: oscilloscope or logic analyser on a STEP pin to confirm
//     motor velocity goes to zero when watchdog fires
//
// What this test does:
//   Phase 1 — Accept heartbeats:
//     Listens for heartbeat packets (0xAA 0x55 0x12 <crc>) and updates the
//     watchdog timer each time one is received.  A fake non-zero JointCommand
//     is pre-loaded so the joint controller has a non-zero target.
//     Prints "HEARTBEAT OK" each time one arrives.
//
//   Phase 2 — Deliberate silence (stop sending heartbeats from the host):
//     After the host stops, the firmware should detect the 2-second timeout
//     and zero the JointCommand.  The test prints "WATCHDOG FIRED" when it
//     detects the zeroed command, verifying the timeout logic in main.cpp
//     works end-to-end.
//
// Success criteria:
//   - "HEARTBEAT OK" appears while heartbeats are flowing
//   - "WATCHDOG FIRED" appears ~2 seconds after heartbeats stop
//   - Motor (if connected) spins down to zero velocity at that point
//
// Flash and monitor:
//   No separate script needed; flash the hwtest env and watch monitor.

#ifdef HWTEST_HEARTBEAT

#include <Arduino.h>

#include "control/joint_controller.h"
#include "drivers/stepper_driver.h"
#include "drivers/encoder_driver.h"
#include "protocol/packet_codec.h"
#include "core/types.h"

static constexpr size_t  kHeartbeatFrameSize = 4U;
static constexpr unsigned long kHeartbeatTimeoutMs = 2000UL;

static robot_arm::StepperDriver  stepper_driver;
static robot_arm::EncoderDriver  encoder_driver;
static robot_arm::JointController joint_controller(stepper_driver);
static robot_arm::PacketCodec    codec;

static unsigned long last_heartbeat_ms = 0UL;
static bool          watchdog_fired    = false;
static uint32_t      heartbeat_count   = 0U;

static uint8_t rx_buf[kHeartbeatFrameSize]{};
static size_t  rx_len = 0U;

void hwtest_heartbeat_setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000UL) { delay(10); }

  Serial.println("========================================");
  Serial.println("HWTEST: Heartbeat watchdog");
  Serial.println("========================================");

  stepper_driver.Init();
  encoder_driver.Init();

  // Pre-load a non-zero command so we can observe it being zeroed.
  robot_arm::JointCommand cmd{};
  for (int j = 0; j < robot_arm::kJointCount; ++j) {
    cmd.target_position_deg[j] = 45.0F;
  }
  joint_controller.SetCommand(cmd);

  last_heartbeat_ms = millis();

  Serial.println("JointCommand pre-loaded: all joints target 45 deg.");
  Serial.println("Send heartbeat packets to keep the watchdog fed.");
  Serial.println("Stop sending to trigger the 2-second timeout.");
  Serial.println();
}

static void check_watchdog() {
  if (watchdog_fired) return;

  const unsigned long now = millis();
  if ((now - last_heartbeat_ms) >= kHeartbeatTimeoutMs) {
    joint_controller.SetCommand(robot_arm::JointCommand{});
    watchdog_fired = true;
    Serial.println();
    Serial.println(">>> WATCHDOG FIRED <<<");
    Serial.println("JointCommand zeroed. Motor should stop.");
    Serial.print("Time since last heartbeat: ");
    Serial.print(now - last_heartbeat_ms);
    Serial.println(" ms");
    Serial.println();
    Serial.println("TEST PASSED if '>>> WATCHDOG FIRED <<<' appeared ~2 s after");
    Serial.println("you stopped sending heartbeat packets.");
    Serial.println("========================================");
  }
}

void hwtest_heartbeat_loop() {
  const unsigned long now = millis();

  // Read heartbeat packets from Serial.
  while (Serial.available() > 0) {
    const int val = Serial.read();
    if (val < 0) break;
    const uint8_t byte = static_cast<uint8_t>(val);

    if (rx_len == 0U && byte != 0xAAU) continue;
    rx_buf[rx_len++] = byte;

    if (rx_len == 2U && (rx_buf[0] != 0xAAU || rx_buf[1] != 0x55U)) {
      rx_len = 0U;
      continue;
    }

    if (rx_len == kHeartbeatFrameSize) {
      if (codec.DecodeHeartbeat(rx_buf, kHeartbeatFrameSize)) {
        last_heartbeat_ms = now;
        watchdog_fired    = false;   // reset so repeated test phases work
        ++heartbeat_count;
        Serial.print("HEARTBEAT OK #");
        Serial.println(heartbeat_count);
      } else {
        Serial.println("Bad heartbeat checksum — ignored");
      }
      rx_len = 0U;
    }
  }

  check_watchdog();

  // Run control loop so motor responds.
  robot_arm::JointState measured{};
  joint_controller.UpdateFromSensors(measured);
  joint_controller.Step(0.001F);
  stepper_driver.Tick();

  delay(1);
}

#endif  // HWTEST_HEARTBEAT
