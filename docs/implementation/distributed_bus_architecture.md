# Distributed Bus Architecture (RS-485 + ESP32 Joint Nodes)

The arm moves from a **centralized** control box (one STM32, long STEP/DIR harnesses) to **distributed joint nodes**: a small **ESP32** next to each motor driver, daisy-chained on a **4-conductor** link through the links.

| Pair | Signals | Purpose |
|:---|:---|:---|
| **A** | `RS485_A`, `RS485_B` | Half-duplex differential bus (twisted pair) |
| **B** | `24V_MOTOR`, `GND_M` | Motor rail + return (gauge per segment current) |

Logic 5V/3.3V is **buck-converted locally** at each node from `24V_MOTOR` (not passed on the 4-wire harness).

---

## Why RS-485 (not CAN)

| Criterion | RS-485 | CAN 2.0 |
|:---|:---|:---|
| ESP32 integration | UART + MAX3485 + DE pin | TWAI peripheral (no extra controller on many boards) |
| Driver ecosystem | CL57T already exposes RS-485 for telemetry | Would not reuse driver UART without adapters |
| Harness | 2-wire differential + termination at ends | Same |
| Software cost | Master-scheduled polling (simple) | Hardware arbitration (slightly more complex frames) |

**Decision:** **RS-485 @ 921600 baud**, master-slave polling from the base **STM32 Nucleo**.

**Also decided:** ESP32 joint nodes may use **Wi-Fi only as an optional service channel** (config, logs, OTA) — **disabled during motion**. See [`joint_node_connectivity.md`](joint_node_connectivity.md).

---

## Topology

```text
[Raspberry Pi] --UART--> [STM32 master @ base] --RS-485-->
      J1 node --> J2 node --> J3 --> J4 --> J5 --> J6 --> Gripper node
      (each: ESP32 + driver + motor phases local)
```

- **Daisy chain:** `BUS_IN` / `BUS_OUT` + `PWR_IN` / `PWR_OUT` on each joint PCB.
- **Termination:** 120 Ω between A/B at the **base master** and at the **last node** (gripper/wrist) only.
- **Rotation (J1):** The same 4 conductors pass through the base swivel or slip ring; bus width stays fixed.

---

## Node map

| Node ID | Role | Motor / actuator |
|:---:|:---|:---|
| `1` | J1 | NEMA 23 + CL57T |
| `2` | J2 | NEMA 23 + CL57T |
| `3` | J3 | NEMA 17 + CL42T |
| `4` | J4 | NEMA 17 + CL42T |
| `5` | J5 | NEMA 14 + TMC2209 |
| `6` | J6 | NEMA 14 + TMC2209 |
| `7` | Gripper | MG996R (PWM) |
| `0` | Master | STM32 (not on bus as a slave) |
| `0xFF` | Broadcast | Heartbeat / discovery |

---

## Frame format

Implemented in [`firmware/lib/bus_protocol/`](../../firmware/lib/bus_protocol/).

```text
[0xAA][0x55][DST][SRC][CMD][LEN][PAYLOAD...][CRC16_LE]
```

- **CRC16-CCITT** over `DST` through end of payload (poly `0x1021`, init `0xFFFF`).
- **Max payload:** 32 bytes. **Max frame:** 40 bytes.

### Command IDs

| CMD | Name | Direction | Payload |
|:---:|:---|:---|:---|
| `0x20` | `SET_JOINT_TARGET` | Master → joint | `position_deg` (f32), `velocity_deg_s` (f32) |
| `0x21` | `JOINT_STATE` | Joint → master | `position_deg`, `velocity_deg_s`, `fault_flags` (u8) |
| `0x22` | `ENABLE` | Master → node | `u8` 1=on, 0=off |
| `0x23` | `HOME` | Master → joint | empty — starts **autonomous homing** on that node (see § Homing) |
| `0x24` | `HEARTBEAT` | Master → broadcast | empty |
| `0x25` | `SET_GRIPPER` | Master → gripper | `duty` f32 (0=open, 1=closed) |
| `0x26` | `GRIPPER_STATE` | Gripper → master | `duty`, `fault_flags` |

Pi ↔ STM32 **UART packets** (`protocol/packet_codec`, `0x10`/`0x11`) are unchanged in role: the Pi still talks only to the base MCU; the STM32 translates to bus transactions.

---

## Homing (Hall sensors on joint nodes)

Homing is **not** executed on the base STM32. Each motor joint has an **A3144 Hall effect sensor** and a fixed magnet at the mechanical zero; the sensor connects to the local **ESP32** (`HOME` GPIO in [`firmware/joint_node/src/pinout.h`](../../firmware/joint_node/src/pinout.h)). The gripper node has no Hall homing — it only reports PWM state.

### Trigger and ownership

| Layer | Role |
|:---|:---|
| **Pi** | Sends `HOME` (`0x04`) on the Pi↔STM32 UART link when the operator or ROS stack requests homing |
| **STM32** | **Orchestrates** only: disables motion commands, issues bus `HOME` (`0x23`) to nodes **in order** J1 → J2 → … → J6 (see [`../Constraints.md §3e`](../Constraints.md)), waits for each node to finish, then replies `HOMED_OK` to the Pi |
| **ESP32 joint node** | **Executes** homing autonomously after a valid addressed `HOME` frame: STEP/DIR sweep, Hall edge detect, stop, zero local step counter / `position_deg` |

Until homing completes system-wide, the STM32 must reject or ignore absolute position commands (policy in [`firmware_architecture.md`](firmware_architecture.md) §2d).

### Per-node homing FSM (ESP32)

When `HOME` (`0x23`) is received for this node's ID:

1. Assert **ENABLE**, set **DIR** toward the configured “seek home” direction, run **STEP** at a low fixed frequency (profile per joint).
2. Poll **`kHomePin`** (A3144, active-low when the magnet is present).
3. On trigger: stop pulses immediately, latch **position = 0°**, clear velocity, set an internal **homed** flag.
4. Respond on the next master poll with `JOINT_STATE` (position near 0, homed indicated in `fault_flags` or equivalent — TBD in firmware).

If **ALARM** asserts or no **HEARTBEAT** within the watchdog window during homing, the node disables the driver and reports fault.

### Sequence (system)

```text
Pi --HOME--> STM32
                |
                +-- HOME node 1 --> [J1 ESP32: sweep + Hall] --> JOINT_STATE (homed)
                +-- HOME node 2 --> [J2 ESP32: ...]
                ...
                +-- HOME node 6 --> [J6 ESP32: ...]
                |
Pi <--HOMED_OK-- STM32
```

Optional post-home move to a safe **rest pose** is a higher-level policy (STM32 streams `SET_JOINT_TARGET` after all nodes report homed).

---

## Timing and rates

- **Baud:** 921600 (adjust if cable length / EMI requires).
- **Master cycle:** Broadcast heartbeat, then poll nodes `1..7` (request + response each).
- **Target aggregate rate:** ~100–200 Hz full-arm updates; nodes interpolate/set STEP locally between polls.
- **Watchdog:** Each slave disables its driver if no valid `HEARTBEAT` (or addressed command) within **500 ms** (`docs/Constraints.md`).

---

## Firmware layout

| Path | Role |
|:---|:---|
| [`firmware/lib/bus_protocol/`](../../firmware/lib/bus_protocol/) | Shared framing + types (STM32 + ESP32) |
| [`firmware/joint_node/`](../../firmware/joint_node/) | ESP32 slave — **one codebase**, ID via env / NVS / straps |
| [`firmware/stm32_core/lib/bus/`](../../firmware/stm32_core/lib/bus/) | RS-485 master scaffold |
| [`firmware/stm32_core/lib/protocol/`](../../firmware/stm32_core/lib/protocol/) | Pi serial protocol (unchanged semantics) |

### Flashing joint nodes

```bash
cd firmware/joint_node
pio run -e node_j4 -t upload    # production: baked-in ID
pio run -e esp32dev -t upload   # bench: then serial `NODE_ID 4`
```

---

## Hardware notes (per node)

- **RS-485 transceiver:** e.g. MAX3485; **DE** high during TX, low for RX.
- **STEP/DIR/ENABLE/ALARM:** ESP32 GPIO → driver (5 V via level shifter where required).
- **Homing:** A3144 on `HOME` pin → ESP32 GPIO only (not routed to the base harness). Magnet placement defines mechanical zero per link.
- **Gripper node:** Dedicated `5V_SERVO` buck; PWM ~50 Hz on ESP32 LEDC.

Placeholder GPIO map: [`firmware/joint_node/src/pinout.h`](../../firmware/joint_node/src/pinout.h).

---

## Migration from centralized wiring

| Before | After |
|:---|:---|
| 6× STEP/DIR/ENABLE/ALARM to control box | Local to each node |
| Motor phases routed to box | Phases **at joint** (short) |
| STM32 bit-bangs all STEP timers | ESP32 per joint; STM32 schedules bus |
| `20_control_signals.yml` long bundles | `40_bus_harness.yml` (4-conductor daisy) |

Centralized firmware paths remain buildable during bring-up; enable distributed mode when `BusMaster` UART backend is wired.

---

## Open work (scaffold)

- [ ] STM32 `BusMaster::Transaction` UART + DE driver
- [ ] ESP32 STEP/DIR timer loop + homing FSM
- [ ] Gripper LEDC mapping
- [ ] Wireviz + KiCad net names for `RS485_A/B`, daisy connectors
- [ ] `hw_interface` assumes aggregated joint state from STM32 (no Pi bus exposure)
- [ ] Optional Wi-Fi service mode per [`joint_node_connectivity.md`](joint_node_connectivity.md)
