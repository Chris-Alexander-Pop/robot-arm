# Firmware contributor guide

This tree is deliberately layered for learning. **`JointController::Step`** is already **P-only** (`kDefaultKp = 2.0F`, Ki/Kd = 0, velocity cap 120 °/s in [`stm32_core/lib/control/src/joint_controller.cpp`](stm32_core/lib/control/src/joint_controller.cpp)). The **native** `StepperDriver` still only stores commanded velocity; the Arduino path uses AccelStepper ([`stm32_core/lib/drivers/src/stepper_driver.cpp`](stm32_core/lib/drivers/src/stepper_driver.cpp)). Protocol maths, **`JointMotionLimits`**, and native unit tests ship as runnable references.

## Tier 1 (begin here)

Hardware motion still to close:

1. **STEP/DIR on the MCU** — follow the numbered **`TODO(contributor)`** checklist in the native half of **`stepper_driver.cpp`**, [`pinout.h`](stm32_core/src/pinout.h), PlatformIO **`stm32_core/platformio.ini`**. AccelStepper (GPL-3.0 or commercial) already drives J1/J2 when `ARDUINO` is set; laurb9/StepperDriver (MIT) is declared for helpers.
2. **Soft limits / cable sanity** — `JointMotionLimits` + `ClampJointCommand` are implemented; practise tuning per joint (`SetMotionLimits`, `AlignLimitsToMeasured` after homing).
3. **Do not treat Ki/Kd as live** — they are 0. Adding integral/derivative is a later tuning job, not the current contract.

Run [`firmware/scripts/test.sh`](scripts/test.sh) (`pio run -e native` + native harness) after each step.

## Tier 2+ (staged product work)

Pick these paths once Tier 1 works on one joint or passes Renode smoke:

- **Real-time stepping**: replace library stepping with hardware **TIM**/ISR pulse trains (aligned with prose in [`docs/implementation/firmware_architecture.md`](../docs/implementation/firmware_architecture.md)).
- **Fault handling**: latch CL57T/CL42T **ALM** inputs (see **`TODO(contributor)`** in [`pinout.h`](stm32_core/src/pinout.h)), optional global DISABLE, propagate a compact fault bitmask in outbound frames beside [`packet_codec`](stm32_core/lib/protocol/src/packet_codec.cpp).
- **Protocol growth**: bounded opcodes (`SET_PID`, soft-limit upload, `FAULT_CLEAR`) with the same XOR checksum framing as today's joint command.
- **Control hygiene**: anti-windup in [`pid_controller.cpp`](stm32_core/lib/control/src/pid_controller.cpp); per-joint gain tables persisted to NVM later.
- **Homing FSM**: sequence drivers to a repeatable datum then call **`AlignLimitsToMeasured`** from [`joint_limits.h`](stm32_core/lib/core/include/core/joint_limits.h); surface status on serial.
- **Multi-axis coupling**: interpolate or synchronise pulses across axes (classic Bresenham step multipliers or streamed segments from the Pi).
- **ROS companion work**: Cartesian planning that prefers low cable winding lives in **`software/ros2_ws`** (TODOs under `robot_core`); firmware remains the realtime backstop (`JointMotionLimits`).

Roadmap bullets in [`firmware/todo.md`](todo.md) are the thematic backlog behind Tier 2+; see the synchronization note at the top of that file.
