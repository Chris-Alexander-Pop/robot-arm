# RS-485 wiring with few dupont cables

Dupont names describe the **end that plugs onto a pin**:

| Cable | Ends | Use when |
|:---|:---|:---|
| **M–F** | pin + socket | Board has **male** pins (Uno, Nucleo Arduino header) |
| **F–F** | socket + socket | Both sides are **male** pins (two modules with pins up) |
| **M–M** | pin + pin | Breadboard holes, or two female headers |

**Male board pin** → use the **female (socket)** end of an M–F or F–F wire.

You only have **one F–F**? Use it for the **long** hop (see below). Everything else should be **M–F** onto the Arduino/Nucleo headers, or **breadboard jumpers** (no dupont).

---

## Best approach: breadboard (fewest dupont wires)

1. Plug each **MAX485** module into a **breadboard** (pins straddle the center gap).
2. Link **5V / GND / DI / RO / DE** with **short solid jumper wires** on the breadboard (the U-shaped wires that often come with the breadboard — not dupont).
3. Use **only 3 dupont wires** between the two breadboards (bus):
   - **A** (M–F or cut solid wire)
   - **B** (M–F or cut solid wire)
   - **GND** ← good use for your **one F–F** if both ends are male **GND** pins on Nucleo/Uno

4. From **Arduino** to its breadboard: **5× M–F** (or 2 power wires + 3 signal if power is on breadboard from Arduino 5V/GND with M–F).

5. From **Nucleo** to its breadboard: **5× M–F** to the Arduino header row (all **male** pins).

---

## Pin map (all on male headers — M–F friendly)

### Arduino Uno / Nano → MAX485

| MAX485 | Arduino | Wire type |
|:---|:---|:---|
| VCC | **5V** | M–F |
| GND | **GND** | M–F |
| DI | **D9** | M–F |
| RO | **D8** | M–F |
| DE (+ RE) | **D7** | M–F |
| A, B | To other module (see bus) | 2 wires |

**D7, D8, D9** are beside each other on the Uno — run the three signal wires as a tight group.

### Nucleo-F401RE → MAX485 (firmware uses these pins)

| MAX485 | Nucleo (Arduino header) | STM32 pin |
|:---|:---|:---|
| VCC | **3.3V** | — |
| GND | **GND** | — |
| DI | **D8** | PA9 |
| RO | **D2** | PA10 |
| DE (+ RE) | **A2** | PA4 |
| A, B | To other module | — |

**Do not use morpho (CN10/CN12) pins** unless you have morpho dupont — use **D8, D2, A2, 3.3V, GND** on the side Arduino connector only.

`D2` is digital pin 2 on the silkscreen (not “D02”). `A2` is analog pin 2.

### Bus between the two MAX485 boards (minimum 3 connections)

| Signal | Connect |
|:---|:---|
| **A** ↔ **A** | 1 wire |
| **B** ↔ **B** | 1 wire |
| **GND** ↔ **GND** | 1 wire (**use your F–F here** if both GND points are male pins) |

**Minimum to test:** 3 bus wires + 5 wires per board to its MCU = **13** connections total. A breadboard cuts that to **3 bus duponts + a few M–F to each board**.

---

## If you truly only have one jumper wire right now

You **cannot** run the full test with one wire. Minimum for *any* bus activity:

1. **Common GND** between Nucleo, Arduino, and both modules (your **one F–F** can do Nucleo GND ↔ Arduino GND).
2. Still need **A**, **B**, and **five wires per MCU↔module** — borrow M–M breadboard jumpers, solid core wire, or a cheap **M–F 40-pin dupont set** (~$2).

**Priority order** if you can only add wires one at a time:

1. **GND** (common reference) — use the F–F between Nucleo and Arduino **GND**
2. **A** and **B** between modules
3. Power + UART on each side (VCC, DI, RO, DE)

---

## Wire gender cheat sheet (this bench)

```text
[Nucleo male pin D8] ←── socket end of M–F ──→ [MAX485 male pin DI]
```

Wrong: **pin end of M–M** onto Nucleo male pin (won’t stay on).

Wrong: **F–F** onto a **female** screw-terminal only module (use bare wire in the screw terminal).

---

## No breadboard?

- Solder **solid core wire** (22–26 AWG) to the MAX485 module pads, or
- Hold **M–F** wires by pressing the socket onto the pin with a small dab of tape (bench only).

---

## After rewiring

Re-flash both boards (pins changed on Nucleo):

```sh
cd firmware/scripts/hardware_tests
./run_rs485.sh
```

Arduino sketch unchanged (still D7/D8/D9).
