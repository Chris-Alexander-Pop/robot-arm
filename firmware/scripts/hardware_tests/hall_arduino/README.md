# Hall sensor test — Arduino (PlatformIO)

Digital **A3144-style** Hall switch test. Same homing idea as the arm: **LOW = magnet present**.

---

## Wiring (bare A3144, front face)

| A3144 pin | Arduino Uno |
|:---|:---|
| **1 (VCC)** | **5V** |
| **2 (GND)** | **GND** |
| **3 (OUT)** | **A0** |

```text
5V ──────── pin 1 (VCC)
GND ─────── pin 2 (GND)
A0 ──────── pin 3 (OUT)     firmware enables internal pull-up
```

### 10 kΩ resistor?

| Situation | Need 10 kΩ (5V → OUT)? |
|:---|:---|
| Bare chip + this sketch (`INPUT_PULLUP` on A0) | **Try without first** |
| OUT stuck LOW with no magnet | **Yes** — 10 kΩ from **5V to OUT** |
| KY-003 / module PCB | **No** — usually on the board |

Your diagram’s resistor goes **VCC to OUT** (pull-up), **not** OUT to GND.

### 4-pin module

| Module | Arduino |
|:---|:---|
| VCC | 5V |
| GND | GND |
| **DO** / D0 | **A0** |
| AO | *(leave unconnected)* |

---

## Flash and run

```sh
cd firmware/scripts/hardware_tests
chmod +x run_hall_arduino.sh   # once
./run_hall_arduino.sh uno        # or: nano | mega
```

USB serial **115200**.

---

## Pass criteria

No magnet:

```text
[status] magnet far  digital=HIGH  adc=1023 (~500 mV@5V)  edges=0
```

Magnet close:

```text
[edge #1] MAGNET NEAR (HOME)  digital=LOW  adc=0
[status] MAGNET NEAR (HOME)  digital=LOW  adc=...
```

**Pass:** `digital` flips **HIGH ↔ LOW** when the magnet enters/leaves.

---

## Tips

- Flip or move the magnet — A3144 only triggers within a few mm.
- `adc` will sit near **0** or **1023**, not a smooth in-between (that is normal for A3144).
- If upload fails with permission denied: `sudo usermod -aG uucp $USER` then log in again.
