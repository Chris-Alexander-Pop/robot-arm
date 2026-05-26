# CL57T bench test — Arduino (speed sweep)

Arduino port of the Nucleo test in
[`hwtest_cl57t_bench.cpp`](../../../stm32_core/test/hardware/hwtest_cl57t_bench.cpp) /
[`run_cl57t_bench.sh`](../run_cl57t_bench.sh).

**Sequence after you type `GO`:**

1. Slow **creep** forward (1 step/s, 5 s)  
2. For each speed **1..12**: run **forward 2 s**, then **reverse 2 s**  
3. Print `BENCH COMPLETE`

---

## Wiring (same as homing bench)

### Through 74HCT541 (recommended)

| Arduino | 74HCT541 | CL57T |
|:---|:---|:---|
| **D2** | A0 → Y0 | **PUL+** |
| **D3** | A1 → Y1 | **DIR+** |
| **D4** | (optional) | **ENA+** |
| **GND** | GND, /OE1, /OE2 | **PUL−, DIR−, COM** |

- Motor PSU **24–48 V** on driver **P4**  
- **S3 = 5 V** logic on CL57T  
- Motor encoder → driver **P2** only (not Arduino)

### Pin summary

| Signal | Arduino Uno |
|:---|:---|
| STEP | **D2** |
| DIR | **D3** |
| ENA | **D4** (LOW = enabled) |

---

## Run

```sh
cd firmware/scripts/hardware_tests
./run_cl57t_bench_arduino.sh uno
```

Serial **115200** → type **`GO`** when the shaft can spin freely.

---

## Tuning

Edit top of `src/main.ino`:

- `kStepsPerRev` — match CL57T microstep DIP (often **1600**)  
- `kDwellMs` — seconds per forward/reverse leg (default **2000**)  
- `kSpeeds[]` — same table as the STM32 bench  

---

## Safety

- Start at low speeds; high steps/s (9–12) are fast.  
- Keep hands clear; be ready to cut motor PSU power.  
- Red **ALM** on driver = fault — power-cycle before retry.
