# Firmware

Domain-level firmware for low-level embedded control: a **base STM32 master** and **ESP32 joint nodes** on a shared **RS-485** bus.

## Projects

| Directory | MCU | Role |
|:---|:---|:---|
| [`stm32_core/`](stm32_core/) | Nucleo-F401RE | Bus master, Pi UART protocol, future motion supervisor |
| [`joint_node/`](joint_node/) | ESP32 (WROOM-32 class) | Per-joint / gripper slave — **one firmware image**, multiple PlatformIO envs |
| [`lib/bus_protocol/`](lib/bus_protocol/) | — | Shared bus framing (linked by both projects) |

Architecture: [`docs/implementation/distributed_bus_architecture.md`](../docs/implementation/distributed_bus_architecture.md) · connectivity policy: [`docs/implementation/joint_node_connectivity.md`](../docs/implementation/joint_node_connectivity.md).

## Onboarding

- Contributor guide: **[CONTRIBUTING.md](CONTRIBUTING.md)**.
- Do **not** commit `compile_commands.json` under either PlatformIO project (see root `.gitignore`).
- Open the **specific** folder (`stm32_core/` or `joint_node/`) as the PlatformIO project root in VS Code.

## Build

Linux/macOS:

```bash
./firmware/scripts/build.sh              # STM32 + all joint_node envs
./firmware/scripts/build.sh stm32        # STM32 only
./firmware/scripts/build.sh joint        # joint_node default env only
./firmware/scripts/build.sh joint-all    # all node_j* + node_gripper envs
```

Windows: `firmware/scripts/build.ps1` with the same optional arguments.

## Test

```bash
./firmware/scripts/test.sh    # native unit tests (STM32 tree, includes bus_protocol)
```

## Flash joint nodes

```bash
cd firmware/joint_node
pio run -e node_j3 -t upload
pio device monitor
```

Unconfigured bench builds: `pio run -e esp32dev -t upload`, then serial command `NODE_ID 3`.

## Scope

- Real-time STEP/DIR and homing at the **joint node** (ESP32).
- RS-485 command/telemetry between base and nodes.
- Pi ↔ STM32 binary protocol unchanged in purpose (`lib/protocol/`).
