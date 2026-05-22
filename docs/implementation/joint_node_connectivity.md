# Joint Node Connectivity (RS-485 Control + Optional Wi-Fi Service)

This document records the **chosen** connectivity model for ESP32 joint nodes: **wired RS-485 for all real-time arm behavior**, with **optional Wi-Fi** used only for configuration, logging, and firmware updates. Motion, homing, and safety watchdogs **never** depend on Wi-Fi.

Related: [`distributed_bus_architecture.md`](distributed_bus_architecture.md) (bus framing, homing), [`firmware_architecture.md`](firmware_architecture.md) (STM32 ↔ Pi gateway).

---

## Decision summary

| Plane | Medium | When active | Carries |
|:---|:---|:---|:---|
| **Control** | RS-485 (UART + MAX3485) | Whenever the arm is powered for operation | `SET_JOINT_TARGET`, `HOME`, `ENABLE`, `HEARTBEAT`, `JOINT_STATE`, faults |
| **Service** | Wi-Fi (ESP32-C3 built-in) | Bench / maintenance / explicit **service mode** only | Node ID, Wi-Fi credentials, log stream, telemetry snapshot, **OTA** |

**Rejected for this project (as the primary arm link):**

- Pi ↔ nodes over Wi-Fi or BLE for motion streaming (latency, EMI, association failures, extra power).
- Replacing the STM32 bus master with a wireless-only stack.

---

## Architecture

```text
                    [ Raspberry Pi ]
                           | UART (control path for ROS / MoveIt)
                           v
                    [ STM32 @ base ] ---- RS-485 bus ----> [ ESP32 node 1..7 ]
                           |                                    |
                     (no Wi-Fi)                          RS-485: motion / homing / WD
                                                           Wi-Fi: OFF during motion
                                                           Wi-Fi: ON in service mode
```

- The **Pi never talks RS-485 directly** and **does not send motion setpoints over Wi-Fi** in normal operation.
- Each **ESP32** may join a Wi-Fi network (or soft-AP for provisioning) for **non-real-time** tools talking to that node or to a small HTTP/MQTT service on the node.

---

## Operational modes (per joint node)

| Mode | RS-485 | Wi-Fi radio | Motor driver | Typical use |
|:---|:---|:---|:---|:---|
| **MOTION** | Active — obey bus master | **Off** (or associated but **service stack disabled**) | Enabled per bus `ENABLE` / watchdog | Homing, MoveIt trajectories, production runs |
| **SERVICE** | Idle or diagnostic-only polls | **On** — provisioning / logs / OTA | **Disabled** unless explicit bench jog via serial | Flash firmware, set `NODE_ID`, tune gains, read logs |
| **FAULT** | Report fault on bus; hold/disable | Off preferred | Disabled | After ALARM, watchdog timeout, or failed homing |

### Entering MOTION mode

Triggered when the STM32 begins normal bus mastering (e.g. after system homing completes, or when the Pi sends an explicit “run” policy — TBD in `stm32_core` state machine):

1. Node closes or ignores Wi-Fi service sockets.
2. Node calls ESP-IDF to **stop Wi-Fi** (or leaves association but sets `wifi_service_enabled = false`) to reduce EMI and current draw near motor drivers.
3. Node relies solely on RS-485 for commands; **500 ms bus watchdog** remains in force ([`Constraints.md`](../Constraints.md)).

### Entering SERVICE mode

Triggered only by a **deliberate** action, not automatically on boot:

- USB serial command, e.g. `SERVICE_MODE 1` on the node (bench).
- Bus command from STM32 (optional future `CMD` — e.g. `0x27 SERVICE_MODE`) when the arm is known idle and Pi has estopped motion.
- Physical “not in run” policy: Pi service daemon puts the arm in maintenance and STM32 stops motion polls.

In SERVICE mode the node **must not** accept motion setpoints over Wi-Fi. OTA and config are allowed; driver ENABLE defaults **off**.

---

## What runs on each plane

### RS-485 (control plane) — authoritative

All semantics in [`distributed_bus_architecture.md`](distributed_bus_architecture.md):

- Target angles / velocities (`SET_JOINT_TARGET`)
- Homing (`HOME`) with local Hall FSM
- Enable/disable and **heartbeat watchdog**
- Gripper duty (`SET_GRIPPER`)
- Aggregated telemetry the STM32 forwards to the Pi

If RS-485 and Wi-Fi disagree, **RS-485 wins** whenever the node is in MOTION mode.

### Wi-Fi (service plane) — optional

Intended capabilities (firmware mostly **planned**):

| Feature | Examples | Notes |
|:---|:---|:---|
| **Provisioning** | Store SSID/password, hostname `joint3.local`, mDNS | First boot or after `NODE_ID` change |
| **Config** | Read/write NVS: node ID, homing direction, step/mm, PID-ish scalars | Mirror of USB serial where possible |
| **Logs** | `/log` or MQTT topic `robot_arm/j3/log` — ring buffer, WARN/ERROR | Rate-limited; no high-frequency joint spam |
| **Telemetry snapshot** | Last known position, fault flags, uptime, RSSI | **Read-only**; not closed-loop control |
| **OTA** | HTTPS or Arduino OTA in SERVICE mode only | Abort OTA if bus enters MOTION / ENABLE asserted |

**Explicitly not on Wi-Fi:** streaming `SET_JOINT_TARGET` at MoveIt rates, `HOME` orchestration, or replacing `HEARTBEAT`.

---

## Network topology (bench vs arm)

| Setup | Recommendation |
|:---|:---|
| **Bench** | Home router or Pi-hosted AP; nodes get DHCP; developer laptop hits `http://jointN.local` or MQTT |
| **On-arm / noisy EMI** | Prefer **no Wi-Fi during motion**; if SERVICE mode on-robot, use a **dedicated 2.4 GHz SSID** with Pi as AP, low transmit power, antenna away from motor cables |
| **Production floor** | RS-485 only during runs; Wi-Fi for maintenance windows |

Credentials live in **NVS per node**, not in the shared firmware binary (except optional compile-time default SSID for lab).

---

## Power and RF

- Wi-Fi active adds **~80–150 mA** per node vs UART-only — significant for seven local 5 V bucks. **Turning Wi-Fi off in MOTION mode** is required for thermal and EMI margin, not optional polish.
- Metal arm structure attenuates 2.4 GHz; treat wireless as **short-range bench**, not guaranteed through the whole workspace.

---

## Software layout (planned)

| Component | Path / note |
|:---|:---|
| Bus slave (existing scaffold) | [`firmware/joint_node/src/joint_node_app.*`](../../firmware/joint_node/src/joint_node_app.*), [`bus_protocol`](../../firmware/lib/bus_protocol/) |
| Mode supervisor | New: `node_mode.h` — `kMotion` / `kService` / `kFault` |
| Wi-Fi service | New: `wifi_service.cpp` — HTTP or MQTT **only compiled/enabled** when `ROBOT_ARM_WIFI_SERVICE=1` |
| USB serial | Remains fallback when Wi-Fi unavailable |

Build flags (illustrative):

- `ROBOT_ARM_WIFI_SERVICE=1` — include service stack
- `ROBOT_ARM_WIFI_OFF_IN_MOTION=1` — default policy

Pi-side (optional later): maintenance tool in `software/` or a small script — **not** part of `hw_interface` / MoveIt real-time path.

---

## Safety rules (HARD policy)

1. **No motion commands accepted over Wi-Fi** in any mode (service API is config/log/OTA only).
2. **Bus watchdog** remains active in MOTION mode; loss of STM32 heartbeat disables the driver even if Wi-Fi is up in violation of policy.
3. **OTA must not run** while the node reports MOTION or driver ENABLED via bus.
4. Entering SERVICE mode should **disable** or **block** driver ENABLE unless on a flagged bench jig.

Documented for alignment with [`Constraints.md` §3e, §4a`](../Constraints.md).

---

## Comparison (why not Wi-Fi-only?)

| Topic | RS-485 control | Wi-Fi motion (rejected) |
|:---|:---|:---|
| Determinism | Master-scheduled poll, predictable | Packet loss, jitter, stack retries |
| EMI environment | Differential pair | 2.4 GHz fade near motors / steel |
| Boot time | Milliseconds | Association + DHCP |
| Pi role | Stays gateway via STM32 | Pi becomes single point + Linux scheduling |
| Harness | One twisted pair in daisy chain | Only power — but RF reliability cost |

Wi-Fi remains valuable where **cables are awkward for humans** (config, OTA), not where **milliseconds matter** (motion).

---

## Open work

- [ ] `NodeMode` state machine in `joint_node` (MOTION ↔ SERVICE)
- [ ] `wifi_service`: provisioning + optional HTTP `/health`, `/config`, OTA endpoint
- [ ] Policy hook from STM32 (optional bus cmd) to force SERVICE on all nodes before maintenance
- [ ] Document Pi maintenance workflow (connect to lab SSID, OTA all nodes)
- [ ] Measure Wi-Fi off vs on current on joint buck (validate 5 V regulator sizing)
