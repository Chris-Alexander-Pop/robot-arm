# Testing

Commands below mirror [`.github/workflows/ci.yml`](.github/workflows/ci.yml).
First-time setup: [`./setup.sh`](setup.sh).

## Verification matrix

| Layer | Local command | CI job |
|-------|---------------|--------|
| Firmware (host / native) | `./firmware/scripts/test.sh` | `firmware-native-tests` |
| Firmware (Renode smoke) | Build `nucleo_f401re_renode`, run `simulation/renode/tests/firmware_smoke.robot` | `firmware-build-and-renode` |
| ROS / MoveIt behavioral | `./software/scripts/test.sh` (requires Docker) | `software-behavioral-tests` |
| Simulation (Python) | `./simulation/scripts/test.sh` | `simulation-tests` |

**Requirements:** PlatformIO (`pio`), Docker for ROS tests, Python 3.11+ for simulation.

## Firmware native tests

Runs PlatformIO `native` environment and the C++ harness under
`firmware/stm32_core/test/` (packet codec, PID, joint limits, driver scaffolds, etc.).

Some tests document **intentional scaffolds** (`TODO(contributor)`, `PENDING`) — see
[`firmware/CONTRIBUTING.md`](firmware/CONTRIBUTING.md).

## Hardware tests (manual, not in CI)

On-target scripts under `firmware/scripts/hardware_tests/` — run only on the bench
with motors powered and safeguards in place.

## Expected scaffold behavior

- **Simulation FK/IK:** pytest may assert placeholder behavior until the DH chain is complete.
- **JointController::Step:** partial implementation; native tests encode the contract.

These are not CI failures by design until the corresponding tier is implemented.
