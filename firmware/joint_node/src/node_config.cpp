#include "node_config.h"

#include <Arduino.h>
#include <Preferences.h>

#include "bus_protocol/bus_types.h"
#include "pinout.h"

namespace joint_node {

namespace {

constexpr const char* kPrefsNamespace = "robot_arm";
constexpr const char* kPrefsNodeIdKey = "node_id";

uint8_t ReadStrapId() {
  pinMode(kIdStrapPin0, INPUT_PULLUP);
  pinMode(kIdStrapPin1, INPUT_PULLUP);
  pinMode(kIdStrapPin2, INPUT_PULLUP);

  const uint8_t bit0 = digitalRead(kIdStrapPin0) == LOW ? 1U : 0U;
  const uint8_t bit1 = digitalRead(kIdStrapPin1) == LOW ? 2U : 0U;
  const uint8_t bit2 = digitalRead(kIdStrapPin2) == LOW ? 4U : 0U;
  const uint8_t encoded = static_cast<uint8_t>(bit0 | bit1 | bit2);
  if (encoded == 0U) {
    return 0U;
  }
  return encoded;  // 1..7 when straps are used
}

}  // namespace

uint8_t ResolveNodeId() {
#if defined(ROBOT_ARM_NODE_ID) && (ROBOT_ARM_NODE_ID >= 1) && (ROBOT_ARM_NODE_ID <= 7)
  return static_cast<uint8_t>(ROBOT_ARM_NODE_ID);
#else
  Preferences prefs;
  if (prefs.begin(kPrefsNamespace, true)) {
    const uint32_t stored = prefs.getUInt(kPrefsNodeIdKey, 0U);
    prefs.end();
    if (stored >= 1U && stored <= robot_arm::bus::kBusNodeCount) {
      return static_cast<uint8_t>(stored);
    }
  }

  const uint8_t strap_id = ReadStrapId();
  if (strap_id >= 1U && strap_id <= robot_arm::bus::kBusNodeCount) {
    return strap_id;
  }

  return 0U;  // Unconfigured — serial console can assign via NODE_ID command
#endif
}

bool IsGripperNode(uint8_t node_id) { return node_id == robot_arm::bus::kAddrGripper; }

}  // namespace joint_node
