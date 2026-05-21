# Joint node firmware (ESP32)

Distributed motor nodes: one small **ESP32** module per joint (and one for the gripper), co-located with the stepper driver. Nodes communicate on a **daisy-chained RS-485 bus** (2 twisted pairs for data + 24V/GND for power through the arm).

## One firmware image

All nodes build from the **same source tree**. Identity is selected by:

1. **PlatformIO environment** (`node_j1` … `node_j6`, `node_gripper`) — sets `ROBOT_ARM_NODE_ID` at compile time (recommended for production).
2. **GPIO strap pins** (optional carrier PCB) — 3-bit ID when straps are populated.
3. **NVS** — persist `NODE_ID <n>` from the USB serial console (`esp32dev` / unconfigured flash).
4. **Console** — `NODE_ID 4` at runtime for bench bring-up.

## PlatformIO

Open **`firmware/joint_node/`** as the project root in VS Code.

```bash
cd firmware/joint_node
pio run -e node_j3          # build joint 3
pio run -e node_gripper -t upload
pio device monitor
```

Default env `esp32dev` leaves `ROBOT_ARM_NODE_ID=0` until configured.

## Pin map

See [`src/pinout.h`](src/pinout.h). Revise when joint carrier PCBs are laid out.

## Protocol

Shared library: [`../lib/bus_protocol/`](../lib/bus_protocol/). Full specification: [`../../docs/implementation/distributed_bus_architecture.md`](../../docs/implementation/distributed_bus_architecture.md).

## Status

Scaffold only: bus framing, RS-485 port, enable/watchdog, and command stubs. STEP/DIR motion, homing, and servo PWM are TODO.
