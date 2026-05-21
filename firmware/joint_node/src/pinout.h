#pragma once

// Placeholder pin map for ESP32 joint-node carrier boards.
// Revise when the joint PCB layout is frozen (see docs/implementation/distributed_bus_architecture.md).

namespace joint_node {

// RS-485 transceiver (e.g. MAX3485): UART2 + driver-enable.
constexpr int kRs485UartNum = 2;
constexpr int kRs485TxPin = 17;
constexpr int kRs485RxPin = 16;
constexpr int kRs485DePin = 4;  // HIGH = transmit enabled

// Stepper driver (CL57T / CL42T / TMC2209) — STEP/DIR/ENABLE/ALARM.
constexpr int kStepPin = 18;
constexpr int kDirPin = 19;
constexpr int kEnablePin = 21;
constexpr int kAlarmPin = 22;

// Homing — A3144 Hall (active-low when magnet present).
constexpr int kHomePin = 23;

// Optional strap pins to read node ID at boot (pull low = bit set). Unpopulated = use build flag / NVS.
constexpr int kIdStrapPin0 = 32;
constexpr int kIdStrapPin1 = 33;
constexpr int kIdStrapPin2 = 25;

// Gripper node (ROBOT_ARM_NODE_ID=7): hobby servo PWM.
constexpr int kGripperPwmPin = 13;

constexpr int kStatusLedPin = 2;

}  // namespace joint_node
