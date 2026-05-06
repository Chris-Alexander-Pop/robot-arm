# Hardware Test Scripts

These scripts build and flash individual hardware tests to the Nucleo-F401RE and
open the serial monitor so you can observe the test output.

Each test is self-contained: it runs from Arduino `setup()` / `loop()` with no
host-side orchestration required unless otherwise noted.

---

## Prerequisites

- **PlatformIO CLI** installed and on `$PATH` (`pip install platformio`)
- **ST-Link USB cable** connecting the Nucleo-F401RE to your workstation
- **`openocd`** available (installed by PlatformIO automatically on first use)

---

## Tests

### `run_stepper_single.sh` — Single-axis stepper (NEMA 23 + CL57T)

**What it does:**
Exercises one motor (joint J1) through a sequence of motions:
1. Ramp up to full speed, hold 2 s, ramp down
2. Reverse direction, hold 2 s, ramp down
3. Speed sweep forward: 50, 100, 200, 500, 1000 steps/s — 2 s dwell each
4. Speed sweep reverse
5. Prints status and "OK" for each phase to the serial monitor

**Required hardware:**
- NEMA 23 motor wired to CL57T driver
- CL57T STEP → `kJ1StepPin` (PB0), DIR → `kJ1DirPin` (PB1) (see `src/pinout.h`)
- CL57T powered (24–48 V DC)

**Passing result:**
```
[1] Ramping UP (forward)...
    Holding max speed 2 s...
    Ramping DOWN...
[1] OK
[2] Reversing direction...
...
[3] Speed sweep (forward):
    50 steps/s ... OK
    100 steps/s ... OK
    ...
HWTEST COMPLETE: all stepper single-axis tests passed.
```

**Run:**
```sh
cd firmware/scripts/hardware_tests
./run_stepper_single.sh
```

---

### `run_stepper_all.sh` — All 6 axes sequential jog

**What it does:**
Jogs each joint (J1–J6) forward then reverse in sequence, reporting "OK"
per joint. Use this to verify all STEP/DIR pairs are wired correctly per
`src/pinout.h`.

**Required hardware:**
- All 6 stepper drivers powered and wired (CL57T or CL42T per joint)
- STEP/DIR pins per `src/pinout.h`: J1–J6 on PB0/PB1, PB10/PB11, etc.

**Passing result:**
```
Joint J1: forward jog... reverse jog... OK
Joint J2: forward jog... reverse jog... OK
...
Joint J6: forward jog... reverse jog... OK
HWTEST COMPLETE: all-axes jog finished.
```
If a joint does not move, check wiring, driver power, and ALM status.

**Run:**
```sh
cd firmware/scripts/hardware_tests
./run_stepper_all.sh
```

---

### `run_comms.sh` — Serial protocol loopback

**What it does:**
Listens for packets from the host (Raspberry Pi or PC) and echoes responses:
- `JointCommand` packet (0xAA 0x55 0x10 + 24 bytes payload + checksum = 28 bytes)
  → decodes positions and echoes a `JointState` with those positions reflected back
- `Heartbeat` packet (0xAA 0x55 0x12 0x12 = 4 bytes)
  → echoes a heartbeat back

Use this to verify the STM32↔RPi serial link works end-to-end before running
the full firmware.

**Required hardware:**
- Nucleo-F401RE (USB for ST-Link and optionally for serial monitor)
- A host connected at 115200 baud capable of sending binary frames

**Packet formats:**
| Packet | Bytes | Content |
|---|---|---|
| Heartbeat TX | `AA 55 12 12` | 4 bytes |
| JointCommand TX | `AA 55 10` + 6×float32 (LE) + XOR checksum | 28 bytes |
| JointState RX | `AA 55 11` + 6×float32 position + 6×float32 velocity + checksum | 52 bytes |
| Heartbeat RX | `AA 55 12 12` | 4 bytes |

**Passing result:**
```
[HB #1] Heartbeat received -> echoing back
[CMD #1] JointCommand decoded. Targets (deg): 45.00, 0.00, 0.00, 0.00, 0.00, 0.00
  JointState echoed back.
```

**Run:**
```sh
cd firmware/scripts/hardware_tests
./run_comms.sh
```

---

## Manual flash (without the shell scripts)

You can also flash any hardware test manually by setting
`PLATFORMIO_BUILD_FLAGS` before calling `pio run`:

```sh
cd firmware/stm32_core

# Single-axis stepper test
PLATFORMIO_BUILD_FLAGS="-DHWTEST_STEPPER_SINGLE" \
  pio run -e nucleo_f401re_hwtest -t upload

# All-axes jog
PLATFORMIO_BUILD_FLAGS="-DHWTEST_STEPPER_ALL_AXES" \
  pio run -e nucleo_f401re_hwtest -t upload

# Comms loopback
PLATFORMIO_BUILD_FLAGS="-DHWTEST_COMMS" \
  pio run -e nucleo_f401re_hwtest -t upload

# Heartbeat watchdog (no separate script — flash manually and watch monitor)
PLATFORMIO_BUILD_FLAGS="-DHWTEST_HEARTBEAT" \
  pio run -e nucleo_f401re_hwtest -t upload

# Open monitor (any env)
pio device monitor -e nucleo_f401re_hwtest
```

### Heartbeat watchdog test (`HWTEST_HEARTBEAT`)

There is no dedicated shell script for this test because it requires interaction:

1. Flash the board:
   ```sh
   cd firmware/stm32_core
   PLATFORMIO_BUILD_FLAGS="-DHWTEST_HEARTBEAT" \
     pio run -e nucleo_f401re_hwtest -t upload && pio device monitor -e nucleo_f401re_hwtest
   ```
2. Send heartbeat packets (`AA 55 12 12`) continuously from the host.  
   You should see `HEARTBEAT OK #N` printed each time.
3. Stop sending heartbeats.  After ~2 seconds you should see:
   ```
   >>> WATCHDOG FIRED <<<
   JointCommand zeroed. Motor should stop.
   ```

**Passing result:** `>>> WATCHDOG FIRED <<<` appears ~2 s after the last heartbeat.

---

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Upload fails with "no device found" | ST-Link not connected or driver missing |
| Motor doesn't move | Check STEP/DIR wiring, driver power, ALM LED |
| CL57T ALM LED solid red | Overcurrent or encoder fault — power-cycle driver |
| Serial monitor shows garbage | Wrong baud rate — should be 115200 |
| "Bad checksum" in comms test | Byte ordering or framing error in host sender |
