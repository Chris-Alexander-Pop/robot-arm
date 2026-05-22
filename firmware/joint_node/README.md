# Joint node firmware (ESP32)

Distributed motor nodes: one small **ESP32** module per joint (and one for the gripper), co-located with the stepper driver. Nodes communicate on a **daisy-chained RS-485 bus** (2 twisted pairs for data + 24V/GND for power through the arm).

**Connectivity policy:** RS-485 is the **control plane** (motion, homing, watchdog). **Wi-Fi** is an optional **service plane** (config, logs, OTA) and must be **disabled during motion**. See [`../../docs/implementation/joint_node_connectivity.md`](../../docs/implementation/joint_node_connectivity.md).

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

## Homing (Hall sensor)

Each motor joint (IDs 1–6) has an **A3144** on `kHomePin` (see [`src/pinout.h`](src/pinout.h)). When the base STM32 sends bus command **`HOME`** (`0x23`) to this node's address, the firmware must:

1. Enable the local driver and STEP toward the configured home direction at low speed.
2. Stop on Hall trigger (active-low), zero position / step count, set homed state.
3. Report completion in the next `JOINT_STATE` response to the master poll.

The STM32 sequences homing **J1 → J6**; the Pi only sees `HOME` / `HOMED_OK` on the UART link. The gripper node does not use Hall homing.

## Wi-Fi service mode (planned)

Not implemented yet. When added:

- **MOTION** — Wi-Fi off; only RS-485 + local STEP/DIR.
- **SERVICE** — Wi-Fi on for NVS config, logs, OTA; driver disabled by default; **no** motion over Wi-Fi.

## Status

Scaffold only: bus framing, RS-485 port, enable/watchdog, and command stubs. STEP/DIR motion, **Hall homing FSM**, servo PWM, and **Wi-Fi service mode** are TODO.
