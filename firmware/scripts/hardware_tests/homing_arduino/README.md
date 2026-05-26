# Homing bench test — Arduino + CL57T + Hall

Runs a **single-joint homing sequence** like the ESP32 design in
[`docs/implementation/distributed_bus_architecture.md`](../../../../docs/implementation/distributed_bus_architecture.md):

1. Enable CL57T  
2. Step slowly toward home until **A3144** sees the magnet (**LOW**)  
3. Stop, back off a few steps, set **position = 0**  
4. Print `HOMED OK`

Motor stepping uses the same **pulse + direction** idea as
[`run_cl57t_bench.sh`](../run_cl57t_bench.sh) / `hwtest_cl57t_bench.cpp`.

---

## Important: CL57T “encoder” is NOT wired to Arduino

The **CL57T** is a **closed-loop driver**. The motor’s encoder plugs into the driver (**P2** on the kit), not into the Arduino.

| You connect to Arduino | You do NOT connect to Arduino |
|:---|:---|
| **STEP** (PUL±) | Motor encoder cable |
| **DIR** (DIR±) | RS-485 (separate test) |
| **ENA** (enable) | |
| **ALM** (optional fault in) | |
| **Hall OUT** (A3144) | |

Arduino only generates **STEP/DIR** and reads **Hall** for homing.

---

## Wiring

### A) Hall sensor (A3144) → Arduino

| A3144 | Arduino Uno |
|:---|:---|
| VCC (pin 1) | **5V** |
| GND (pin 2) | **GND** |
| OUT (pin 3) | **A0** |

Optional **10 kΩ** from **5V → OUT** if the line floats (see [`hall_arduino/README.md`](../hall_arduino/README.md)).

Mount the **magnet** at the mechanical “home” position.

### B) Arduino → 74HCT541 → CL57T (same as Nucleo bench)

If you already use a **74HCT541** level shifter (recommended for 5 V CL57T logic):

| Arduino | 74HCT541 | CL57T |
|:---|:---|:---|
| **D2** | A0 → Y0 | **PUL+** |
| **D3** | A1 → Y1 | **DIR+** |
| **D4** | (optional) | **ENA+** |
| **D5** | — | **ALM+** (input to Arduino) |
| **GND** | GND, /OE1, /OE2 | **PUL−, DIR−, COM** |

From [`run_cl57t_bench.sh`](../run_cl57t_bench.sh):

- CL57T **S3 = 5 V** logic  
- Motor PSU **24–48 V** on **P4**  
- Motor phases on driver  
- **Encoder on P2** → driver only  

### C) Arduino → CL57T direct (5 V logic mode only)

If the CL57T is set to **5 V** logic (DIP/jumper per manual):

| Arduino | CL57T |
|:---|:---|
| D2 | PUL+ |
| D3 | DIR+ |
| D4 | ENA+ |
| D5 | ALM+ (optional) |
| GND | PUL−, DIR−, ENA−, COM |

---

## Run

```sh
cd firmware/scripts/hardware_tests
chmod +x run_homing_arduino.sh
./run_homing_arduino.sh uno
```

Serial **115200**. Type:

```text
HOME
```

Expected:

```text
HOME — enabling driver, seeking Hall...
  Hall triggered at step 1234
  Backing off a few steps...

HOMED OK — position_steps = 0
```

Other commands: `STATUS`, `RESET`.

---

## Tuning (top of `src/main.ino`)

| Constant | Meaning |
|:---|:---|
| `kSeekHomeDirHigh` | Flip if motor seeks away from the magnet |
| `kHomingStepsPerSec` | Seek speed — default **4500** steps/s (CL57T bench **speed 10**) |
| `kStepsPerRev` | Match CL57T microstep DIP (often 1600) |
| `kMaxSeekSteps` | Abort if Hall never found |

---

## Troubleshooting

| Symptom | Fix |
|:---|:---|
| Motor never moves | Enable wiring, PSU, **ENA**, 541 /OE grounded |
| Runs forever, no HOMED | Wrong **DIR** → flip `kSeekHomeDirHigh` |
| HOMED immediately | Magnet already at sensor — move axis away first |
| ALM fault | Overcurrent / encoder fault on driver — power-cycle CL57T |
| Hall never triggers | Magnet polarity, distance, or Hall wiring |
