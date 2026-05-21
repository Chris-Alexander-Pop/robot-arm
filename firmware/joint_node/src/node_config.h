#pragma once

#include <stdint.h>

namespace joint_node {

// Resolved at boot: build flag, optional GPIO straps, or NVS key "node_id".
uint8_t ResolveNodeId();

bool IsGripperNode(uint8_t node_id);

}  // namespace joint_node
