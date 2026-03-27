# Electrical Bring-Up Checklist

This checklist is for the first time hardware is available. It is intentionally conservative so the system can be validated without risking the PSU, drivers, or controller.

## 1. Prerequisites

- One PSU
- One driver channel
- One motor
- One home sensor
- One logic power supply
- One current-limited bench setup or fused power path
- Multimeter
- If available, an oscilloscope or logic analyzer

## 2. Pre-Power Checks

- Verify the wiring harness against the interface map.
- Confirm connector pin 1 orientation on every harness.
- Check for continuity between intended nets only.
- Check for shorts between 24V, 5V, and ground.
- Confirm the E-stop really removes power from the motor rail.
- Confirm logic ground and motor ground are connected where intended, and only where intended.

## 3. Power-Up Sequence

1. Power the logic rail only.
2. Confirm STM32 boots.
3. Confirm the UART link to the Raspberry Pi is alive.
4. Confirm the I2C bus sees the mux and encoder hardware if fitted.
5. Power the motor rail with the motor disconnected.
6. Confirm driver idle current and fault state.
7. Connect one motor and repeat the idle check.

## 4. Single-Axis Validation

- Command a small STEP/DIR motion at low speed.
- Verify the motor turns in the expected direction.
- Verify ENABLE disables motion cleanly.
- Verify ALARM stops motion and is reported to firmware.
- Verify the home sensor toggles at the expected mechanical position.
- If using an encoder, verify angle changes match physical rotation.

## 5. Safety Validation

- Press E-stop during idle.
- Press E-stop during motion.
- Unplug the home sensor and verify the system fails safely.
- Force a driver alarm if the driver supports it.
- Disconnect the serial link and confirm the watchdog behavior.
- Restore power and confirm the arm does not move unexpectedly.

## 6. Acceptance Criteria

The first joint is considered healthy when:
- It powers up without fault.
- It homes repeatably.
- It can move a short commanded distance and stop accurately.
- The driver fault line is observed and handled correctly.
- No connector, regulator, or driver runs unreasonably hot during a short test.

## 7. Expansion Rule

Do not bring up the next joint until the current joint passes the checklist above.
