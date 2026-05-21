#include <Arduino.h>

#include "joint_node_app.h"
#include "node_config.h"

joint_node::JointNodeApp g_app;

void setup() {
  Serial.begin(115200);
  delay(500);

  const uint8_t node_id = joint_node::ResolveNodeId();
  if (node_id == 0U) {
    Serial.println("[joint_node] node_id unconfigured — use NODE_ID <1-7> or flash -e node_jN");
  }

  g_app.Setup(node_id);
}

void loop() {
  g_app.Loop();
}
