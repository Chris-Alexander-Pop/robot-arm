# STM32F401RE Pinout Decisions

This document tracks the pin assignment strategy for the STM32F401RE firmware target.

**Board alignment:** Matches `platformio.ini`: **Nucleo-F401RE** (`board = nucleo_f401re`).
It is intentionally a design note, not a generated CubeMX file.
The current firmware code is still a scaffold, so the final pin map should be reviewed
again before hardware bring-up.

## Why This Exists

The STM32F401RE has many multifunction pins, but only a subset can be used at the same
height of the design without conflicts. The important part is not just "what pins exist",
but which peripheral functions must be reserved together:

- SWD debug/programming pins must stay available.
- The host communication link needs a stable UART or USB path.
- Encoder feedback needs a clean I2C bus.
- Six stepper axes need six STEP outputs and six DIR outputs.
- Homing, fault, and emergency-stop signals can quickly consume the remaining GPIO.

That means the pinout should be planned as a budget, not as a collection of isolated pins.

## Current Firmware State

The code in `stm32_core/src/main.cpp` is still a single-axis prototype.
It currently uses placeholder Arduino-style pin definitions and a single AS5600 encoder.
The real multi-axis firmware will likely move the pin assignments into a central header
or board-specific configuration file so that all drivers share one source of truth.

## Design Rules

### 1. Reserve the essential system pins first

These pins should not be repurposed for robot I/O:

- `PA13` / `PA14`: SWDIO and SWCLK for programming and debug
- `NRST`: reset line
- `BOOT0`: boot mode strap

If a board variant exposes a user button or LED on a shared pin, confirm it does not
interfere with the intended robot functions.

### 2. Keep host communication separate from debug

The robot link to the Raspberry Pi should not depend on the same interface used for flashing
or console output unless that is a deliberate choice.
For this project, the preferred pattern is:

- One UART for the robot control link
- SWD kept for programming/debugging
- Optional secondary debug console only if there are enough free pins

This keeps the motion-control path independent of local development convenience.

### 3. Reserve one I2C bus for optional wrist feedback

J1–J4 use closed-loop driver kits whose factory motor encoder is read **inside the driver**; the STM32 never touches I2C for those joints. The shared I2C bus on this design only matters for the **optional** AS5600 add-on at J5 / J6.

Recommended bus selection (kept reserved even when no AS5600 is fitted, so a Phase 2 retrofit is a wiring change rather than a firmware redesign):

- `I2C1` on `PB6` (SCL) and `PB7` (SDA)

Reasons:

- Common on STM32F4 boards
- Easy to route
- Leaves the higher-value pins free for time-critical signals
- A TCA9548A multiplexer is only required if **both** J5 and J6 get an AS5600 (since both share the fixed `0x36` address). With one wrist encoder, the AS5600 sits directly on the bus.

### 4. Keep STEP/DIR pins on plain GPIO unless hardware timers are proven necessary

The current firmware scaffold uses software-stepped motion control.
That means the stepper outputs only need reliable digital outputs for now.
If the design later migrates to timer-driven pulse generation, the pin map should be
rechecked against timer-capable alternate functions.

## Proposed Pin Budget

The table below is the working allocation strategy.
Some entries are firm, and others are still candidates because the final joint count and
fault strategy may change.

| Function | Count | Suggested STM32F401RE Resource | Status |
| --- | ---: | --- | --- |
| SWD debug/programming | 2 | `PA13`, `PA14` | Reserved |
| Host control link | 2 | `USART1` or equivalent external UART | Needed |
| Encoder bus | 2 | `I2C1` on `PB6`, `PB7` | Preferred |
| Step outputs | 6 | Six GPIO outputs | Needed |
| Direction outputs | 6 | Six GPIO outputs | Needed |
| Home sensors | 6 | Six interrupt-capable inputs | Needed |
| Driver alarms | 6 | Six fault inputs, or a grouped fault line | Open |
| E-stop | 1 | Dedicated interrupt input | Needed |
| Spare expansion | 1+ | GPIO or I2C expander | Strongly recommended |

## Recommended Functional Grouping

### Motion outputs

Use one GPIO pair per joint:

- J1: STEP + DIR
- J2: STEP + DIR
- J3: STEP + DIR
- J4: STEP + DIR
- J5: STEP + DIR
- J6: STEP + DIR

This keeps the firmware simple and makes it easy to map each joint to a controller slot.

### Encoder feedback

For J1–J4, position comes from the closed-loop driver itself (RS-485 from the CL57T / CL42T) — no STM32 GPIO is consumed by the driver-side encoder. For J5 / J6, the optional wrist AS5600 (if fitted) lives on `I2C1`. A TCA9548A multiplexer is needed only when both wrist AS5600 boards are populated, since they share the fixed `0x36` address.

### Fault handling

The most likely place for pin pressure is fault handling.
If every driver has its own ALARM line plus a separate home sensor, the board can run out of
clean GPIO faster than expected.
Two practical options exist:

1. Keep each fault line discrete and use more of the GPIO budget.
2. Group less critical faults through an I2C GPIO expander or a wired-OR fault bus.

The second option is usually the safer choice if the board variant proves tight.

## Candidate Board-Level Allocation

This is a practical starting point for review, not a final electrical drawing.
It favors keeping the high-value buses on stable pins and spreading the stepper outputs
across available GPIO banks.

| Signal Group | Candidate Pins | Notes |
| --- | --- | --- |
| SWD | `PA13`, `PA14` | Do not use for robot I/O |
| Robot UART | `PA9`, `PA10` | Good candidate for the Pi link if available |
| I2C1 | `PB6`, `PB7` | Encoder bus |
| J1 STEP/DIR | `PB0`, `PB1` | General GPIO |
| J2 STEP/DIR | `PB10`, `PB11` | General GPIO |
| J3 STEP/DIR | `PB12`, `PB13` | General GPIO |
| J4 STEP/DIR | `PB14`, `PB15` | General GPIO |
| J5 STEP/DIR | `PC6`, `PC7` | General GPIO |
| J6 STEP/DIR | `PC8`, `PC9` | General GPIO |

This layout keeps the core buses separate from the motion outputs and leaves the lower
GPIO range available for sensors and fault inputs.
The exact pin numbers should still be checked against the specific package and board variant
before hardware wiring is finalized.

## Open Decisions

- Whether the host link should use UART or USB CDC
- Whether each motor driver needs its own alarm input or a shared fault line is enough
- Whether all six homing sensors are read directly or through an I2C GPIO expander
- Whether the final firmware uses Arduino-style GPIO control or STM32 HAL/CubeMX configuration
- Whether a board-specific `pinout.h` should be introduced once the final map is settled

## Recommendation

For now, treat this as the working pinout policy:

- Reserve SWD and reset pins permanently.
- Put the Pi link on one UART.
- Put all encoders on one I2C bus.
- Give each joint a dedicated STEP/DIR pair.
- Use the remaining GPIO for homes, alarms, and E-stop.
- Add an I2C GPIO expander if the board runs out of clean interrupt-capable inputs.

That approach fits the STM32F401RE resource limits without overcommitting the design too early.

---

## RS-485 bus master (distributed architecture)

When using ESP32 joint nodes ([`docs/implementation/distributed_bus_architecture.md`](../docs/implementation/distributed_bus_architecture.md)), the Nucleo acts as the **RS-485 master** only — it no longer bit-bangs STEP/DIR for remote joints.

| Function | Suggested pin | Notes |
|:---|:---|:---|
| `RS485_TX` | TBD (`USARTx_TX`) | Through MAX3485 |
| `RS485_RX` | TBD (`USARTx_RX`) | |
| `RS485_DE` | TBD (GPIO) | HIGH = transmit enabled |
| Pi `UART_TX/RX` | `PA9` / `PA10` | Unchanged host link |

**Termination:** 120 Ω at the Nucleo transceiver (bus end). Second terminator at the gripper node.

Joint-node GPIO (STEP/DIR/ENABLE/ALARM/HOME) lives in [`joint_node/src/pinout.h`](joint_node/src/pinout.h).
