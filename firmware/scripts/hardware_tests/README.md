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

### `run_cl57t_bench.sh` — CL57T bench bring-up (PA0 / PA1, very low speed)

**What it does:**
Slow, safe first-motion sequence for the bench wiring (STM32 → 74HCT541 → CL57T):

1. Serial banner, then wait for **B1** (blue USER button on Nucleo)
2. Forward: 10, 25, then 40 steps/s — 2 s each
3. Reverse: same speeds
4. Stops and prints completion message

**Required hardware:**
- PA0 → 541 A0 → Y0 → CL57T PUL+; PA1 → 541 A1 → Y1 → CL57T DIR+
- PUL−, DIR−, COM and 541 GND common; 541 /OE1, /OE2 → GND; CL57T **S3 = 5 V**
- Motor PSU on P4; encoder on P2; **SW6 OFF** (closed loop)

**Run:**
```sh
cd firmware/scripts/hardware_tests
./run_cl57t_bench.sh
```

---

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

### `run_rs485_arduino.sh` — Arduino RS-485 echo (PlatformIO)

**What it does:** Builds and flashes `rs485_arduino/` on an Arduino Uno/Nano/Mega; opens serial monitor at 115200. The sketch echoes bus traffic for the Nucleo ping test.

**Usage:**
```sh
./run_rs485_arduino.sh        # default: uno
./run_rs485_arduino.sh nano
```

**Project:** `firmware/scripts/hardware_tests/rs485_arduino/` (`pio run -e uno -t upload`).

---

### `run_rs485.sh` — RS-485 transceiver (MAX485 / MAX3485)

**Arduino + Nucleo pair:** flash Arduino first with [`run_rs485_arduino.sh`](run_rs485_arduino.sh) (PlatformIO), then run this script. Step-by-step: [`rs485_arduino/QUICKSTART.md`](rs485_arduino/QUICKSTART.md).

**What it does:**
Once per second, drives the RS-485 bus in transmit mode, sends `RS485 PING`, then
listens for any bytes echoed by another node on the bus. Progress is printed on the
USB serial monitor (USART2 / ST-Link), not on the RS-485 line.

**Required hardware:**
- Nucleo-F401RE + ST-Link USB
- 3.3 V MAX485-style TTL module wired to `PC10` (TX), `PC11` (RX), `PA4` (DE)
- Bus partner on `A`/`B`: second MAX485 module — **Arduino** echo sketch
  ([`rs485_arduino/`](rs485_arduino/README.md)) **or** USB-UART adapter on a PC
- Common ground between all modules
- 120 Ω between `A` and `B` at each physical end of the bus (optional on a short bench)

**Run:**
```sh
cd firmware/scripts/hardware_tests
./run_rs485.sh
```

**Passing result:** `TX` lines every second; `RX` lines when a partner echoes or replies.

---

### `run_cl57t_bench_arduino.sh` — CL57T speed sweep (Arduino)

**What it does:** Same as Nucleo `run_cl57t_bench.sh` — creep, then speeds 1–12 **forward then reverse** (2 s each). Type **`GO`** to start.

**Wiring:** [`cl57t_bench_arduino/README.md`](cl57t_bench_arduino/README.md) — **D2** STEP, **D3** DIR, **D4** ENA.

```sh
./run_cl57t_bench_arduino.sh uno
```

---

### `run_homing_arduino.sh` — Homing sequence (CL57T + Hall on Arduino)

**What it does:** Enables CL57T, steps until A3144 Hall triggers, backs off, prints `HOMED OK`. Same idea as production joint-node homing.

**Wiring:** [`homing_arduino/README.md`](homing_arduino/README.md) — STEP/DIR/ENA to CL57T, Hall on **A0**, encoder stays on driver **P2** only.

**Run:** flash then type `HOME` at 115200:

```sh
./run_homing_arduino.sh uno
```

---

### `run_hall_arduino.sh` — Hall sensor on Arduino (A3144, A0)

**What it does:** Reads a digital Hall switch on **A0** (5V, pull-up). Prints `MAGNET NEAR (HOME)` when **LOW**.

**Wiring:** [`hall_arduino/README.md`](hall_arduino/README.md)

**Run:**
```sh
./run_hall_arduino.sh uno
```

---

### `run_hall.sh` — Hall sensor on Nucleo (optional)

Same test on STM32 — only if ST-Link upload works.

**What it does:**
Reads a **digital** Hall switch on Nucleo **A5 (PC0)** with internal pull-up. Prints `MAGNET NEAR (HOME)` when the output is **LOW** (same logic as production joint-node homing).

**Required hardware:**
- A3144 module (or compatible digital Hall board)
- Small magnet
- Wiring: see [`hall-effect-sensor-testing/README.md`](hall-effect-sensor-testing/README.md)

**Run:**
```sh
./run_hall.sh
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

# CL57T bench (PA0/PA1, low speed)
PLATFORMIO_BUILD_FLAGS="-DHWTEST_CL57T_BENCH" \
  pio run -e nucleo_f401re_hwtest -t upload

# Single-axis stepper test
PLATFORMIO_BUILD_FLAGS="-DHWTEST_STEPPER_SINGLE" \
  pio run -e nucleo_f401re_hwtest -t upload

# All-axes jog
PLATFORMIO_BUILD_FLAGS="-DHWTEST_STEPPER_ALL_AXES" \
  pio run -e nucleo_f401re_hwtest -t upload

# Hall sensor
PLATFORMIO_BUILD_FLAGS="-DHWTEST_HALL" \
  pio run -e nucleo_f401re_hwtest -t upload

# Comms loopback
PLATFORMIO_BUILD_FLAGS="-DHWTEST_COMMS" \
  pio run -e nucleo_f401re_hwtest -t upload

# RS-485 transceiver test
PLATFORMIO_BUILD_FLAGS="-DHWTEST_RS485" \
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

Run **`./check_nucleo_stlink.sh`** before any Nucleo upload. It prints whether SWD can reach the MCU.

| Symptom | Likely cause |
|---|---|
| `init mode failed` / `chipid: 0x000` / `NRST is not connected` | **CN2 jumpers off** (most common), wires on **CN4**, or ST-Link section cut off the board — see `check_nucleo_stlink.sh` |
| mbed `FAIL.TXT`: failed to reset/halt target | Same as above — ST-Link cannot reach the STM32 |
| Upload fails with "no device found" | ST-Link not connected or driver missing |
| Motor doesn't move | Check STEP/DIR wiring, driver power, ALM LED |
| CL57T ALM LED solid red | Overcurrent or encoder fault — power-cycle driver |
| Serial monitor shows garbage | Wrong baud rate — should be 115200 |
| "Bad checksum" in comms test | Byte ordering or framing error in host sender |
