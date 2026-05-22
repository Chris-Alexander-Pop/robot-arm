# Electrical Schematic Plan

This is the KiCad-ready block plan for the robot arm electrical system. It describes how the schematic should be split into sheets and what each sheet should contain.

**KiCad project (draft):** [hardware/kicad/robot_arm/robot_arm.kicad_pro](../../hardware/kicad/robot_arm/robot_arm.kicad_pro)

## 1. Top-Level Power Flow

```text
AC mains
  -> IEC C14 inlet + fuse
  -> latching E-stop
  -> Mean Well LRS-350-24
  -> 24V motor bus
  -> motor drivers
  -> logic buck converters
  -> STM32 / sensors / Raspberry Pi 4
```

## 2. Recommended Schematic Sheets

### 2a. Power Entry Sheet

Include:
- IEC C14 inlet
- Fuse holder
- E-stop switch in series with the mains feed
- PSU output terminals
- Main 24V distribution node
- Ground / chassis bonding points

### 2b. Logic Power Sheet

Include:
- 24V to 5V buck converter for logic
- Separate 24V to 5V buck converter for Raspberry Pi 4 (`5V_PI`, ≥3A capable)
- Bulk capacitance on the 5V rail
- Test points for 24V, 5V, and 3.3V

### 2c. STM32 Control Sheet

Include:
- STM32 header / breakout
- 74HC245 level shifter
- UART to Raspberry Pi 4
- RS-485 transceiver to daisy-chained ESP32 joint nodes (see [`distributed_bus_architecture.md`](distributed_bus_architecture.md))
- I2C bus to TCA9548A (optional wrist AS5600 only)
- RS-485 to joint-node daisy chain (no per-joint STEP/DIR at base in distributed mode)
- Pi UART; optional legacy STEP/DIR only for bench bring-up

### 2d. Motor Driver Sheet

Include one repeated channel block per joint:
- Driver power input from 24V rail
- STEP / DIR / ENABLE input signals
- ALARM output
- Motor connector
- Optional encoder connector for closed-loop channels

### 2e. Joint Node Sheet (per link)

Repeat one hierarchical sheet per joint node PCB (ESP32 + local buck + MAX3485 + driver):

- A3144 Hall home sensor header → **ESP32 `HOME` GPIO** (boot homing on the node)
- STEP / DIR / ENABLE / ALARM to the local driver
- RS-485 and 24V daisy in/out
- Optional AS5600 header on **J5/J6 nodes only** (Phase 2)

### 2f. Base Sensor Sheet (optional wrist path)

On the **base** control board only when needed:

- I2C pullups (4.7k to `5V_LOGIC`) for optional wrist AS5600 path (legacy centralized wiring)
- AS5600 + TCA9548A **depopulated by default**

> Production homing does **not** route `HOME_J1`…`HOME_J6` to the Nucleo. Hall sensors live on joint-node boards. Default build: no AS5600s, no mux at the wrist unless drift forces it.

## 3. Net Naming Convention

Use a consistent naming scheme so the schematic remains readable:

- `24V_MOTOR`
- `5V_LOGIC`
- `5V_PI`
- `STM32_STEP_J1` ... `STM32_STEP_J6`
- `STM32_DIR_J1` ... `STM32_DIR_J6`
- `STM32_EN_J1` ... `STM32_EN_J6`
- `DRV_ALARM_J1` ... `DRV_ALARM_J6`
- `NODE_HOME` (per joint-node sheet; not `HOME_Jx` at base in distributed mode)
- `I2C_SCL`, `I2C_SDA`
- `UART_TX`, `UART_RX`

## 4. Design Rules for the Schematic

- Keep motor power separate from logic power until the connector boundary.
- Put all decoupling capacitors near the load they serve.
- Draw the fault path so it is obvious that a driver alarm can disable motion.
- Label every net that crosses a sheet boundary.
- Use repeated hierarchical sheets for the six joints instead of duplicating one-off wiring.
- Keep the schematic human-readable first; PCB optimization comes later.

## 5. KiCad Implementation Order

1. Draw the power-entry and buck-converter sheets.
2. Add the STM32 control sheet and confirm all logic nets.
3. Build one reusable motor-driver sheet for a single joint.
4. Replicate that sheet for all six joints.
5. Add the sensor sheet and verify the I2C and home inputs.
6. Assign footprints only after the schematic netlist is stable.
