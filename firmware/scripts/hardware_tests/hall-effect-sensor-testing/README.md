# Hall sensor bench test (STM32 Nucleo)

Tests an **A3144-style** digital Hall switch the same way production homing will use it on joint nodes: **active-low when the magnet is present**.

Firmware: `hwtest_hall` via PlatformIO (`../run_hall.sh`).

---

## Wiring: A3144 module → Nucleo-F401RE

Typical 3-pin Hall module (labels vary: `VCC`, `GND`, `OUT` / `D0` / `AO`):

| Hall module | Nucleo (Arduino header) | Notes |
|:---|:---|:---|
| **VCC** | **3.3V** | Use 3.3 V on the Nucleo (not 5 V) unless your module is 5 V-only |
| **GND** | **GND** | |
| **OUT** | **A5** | STM32 pin **PC0**; firmware uses `INPUT_PULLUP` |

Use **M–F** dupont wires onto the Nucleo’s male header pins.

### If your board has 4 pins

| Pin | Connect |
|:---|:---|
| VCC | 3.3V |
| GND | GND |
| DO / D0 / digital out | **A5** |
| AO | *(leave unconnected — digital test only)* |

### Bare A3144 chip (no PCB)

| A3144 | Connect |
|:---|:---|
| VCC (pin 1) | 3.3V |
| GND (pin 2) | GND |
| OUT (pin 3) | **A5** + **10 kΩ** to 3.3V (pull-up) |

Add a **0.1 µF** cap near the chip between VCC and GND if the reading is noisy.

---

## Is `raw` analog? Do I need the 10 kΩ resistor?

### Analog vs digital (A3144)

The **A3144 is not an analog distance sensor.** It is a **digital switch**:

| Magnet | OUT (digital) | ADC on same pin (debug only) |
|:---|:---|:---|
| Far | **HIGH** | ~**4095** (3.3 V) |
| Near | **LOW** | ~**0** |

You will **not** get a smooth 0→4095 value as you move the magnet closer. Homing only needs **HIGH vs LOW**.

The test prints both `digital=LOW/HIGH` (use this) and `adc=0..4095` (shows it is basically two levels).

If you have a **linear analog Hall** (e.g. SS49E) on **AO**, that is a different sensor — wire **AO** to an analog pin and we would need a different test.

### 10 kΩ pull-up resistor (VCC → OUT)

**Not absolutely required** for this bench test if:

- You power the chip from **3.3 V**, and  
- You flash `run_hall.sh` (firmware enables **internal pull-up** on A5).

**Add** a **10 kΩ from 3.3 V to OUT** (same as your diagram, but use **3.3 V** not 5 V) if:

- `digital` is stuck **LOW** with no magnet, or  
- The line floats (random toggling).

Many **breakout boards** already include that resistor — then you do **not** add another.

**Do not** power OUT at **5 V** levels into the Nucleo **A5** pin — use **3.3 V** for VCC and pull-up.

---

## Run the test

```sh
cd firmware/scripts/hardware_tests
chmod +x run_hall.sh   # once
./run_hall.sh
```

USB serial monitor **115200 baud**.

---

## What you should see

With no magnet nearby:

```text
[status] magnet far  digital=HIGH  adc=4095 (~3300 mV)  triggers=0
```

Move a magnet close to the sensor:

```text
[edge #1] MAGNET NEAR (HOME)  digital=LOW  adc=0
[status] MAGNET NEAR (HOME)  digital=LOW  adc=12 (~9 mV)  triggers=1
```

Move the magnet away:

```text
[edge #2] magnet far
```

**Pass:** `digital` toggles **HIGH ↔ LOW** when the magnet enters/leaves the trigger zone.  
**Fail:** stuck on one level — check VCC/GND, wrong pin, or a module that needs 5 V.

---

## Tips

- **Polarity:** Flip the magnet if the sensor never triggers.
- **Distance:** A3144 usually needs to be within a few millimetres of the magnet.
- **Pin conflicts:** Do not use **A0/A1** if the CL57T bench test is wired there; this test uses **A5** only.
- **Production:** On the real arm, Hall sensors connect to **ESP32 joint nodes**, not this Nucleo pin — see [`firmware/joint_node/src/pinout.h`](../../../joint_node/src/pinout.h).
