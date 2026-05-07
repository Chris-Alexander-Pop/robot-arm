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
- I2C bus to TCA9548A
- STEP / DIR / ENABLE outputs
- Fault inputs from driver ALARM pins
- Home sensor inputs

### 2d. Motor Driver Sheet

Include one repeated channel block per joint:
- Driver power input from 24V rail
- STEP / DIR / ENABLE input signals
- ALARM output
- Motor connector
- Optional encoder connector for closed-loop channels

### 2e. Sensor Sheet

Include:
- A3144 Hall home sensor headers — **one per joint, J1–J6** (always populated; used for boot-time homing)
- I2C pullups (4.7k to `5V_LOGIC`) on the controller side
- Optional shield termination
- AS5600 + TCA9548A footprints **only as Phase 2 / depopulated parts**, intended for J5 and J6 only. The closed-loop kits on J1–J4 do their own encoder feedback inside the driver, so do not add AS5600 headers for those joints.

> Default build: no AS5600s, no mux. Only stuff the I2C path if the wrist actually needs absolute feedback.

## 3. Net Naming Convention

Use a consistent naming scheme so the schematic remains readable:

- `24V_MOTOR`
- `5V_LOGIC`
- `5V_PI`
- `STM32_STEP_J1` ... `STM32_STEP_J6`
- `STM32_DIR_J1` ... `STM32_DIR_J6`
- `STM32_EN_J1` ... `STM32_EN_J6`
- `DRV_ALARM_J1` ... `DRV_ALARM_J6`
- `HOME_J1` ... `HOME_J6`
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
