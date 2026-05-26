# Arduino RS-485 bus partner (PlatformIO)

Echo firmware for bench-testing a **MAX485** with the STM32 Nucleo
[`run_rs485.sh`](../run_rs485.sh).

**Dupont / wiring help:** [WIRING_DUPONT.md](WIRING_DUPONT.md)  
**Step-by-step:** [QUICKSTART.md](QUICKSTART.md)

---

## PlatformIO

Project root: **`firmware/scripts/hardware_tests/rs485_arduino/`**

| Environment | Board |
|:---|:---|
| `uno` (default) | Arduino Uno |
| `nano` | Arduino Nano (ATmega328P) |
| `mega` | Arduino Mega 2560 |

```sh
cd firmware/scripts/hardware_tests
./run_rs485_arduino.sh uno
```

```sh
cd firmware/scripts/hardware_tests/rs485_arduino
pio run -e nano -t upload
pio device monitor -e nano
```

Source: `src/main.ino`

---

## Wiring: MAX485 ↔ Arduino

| MAX485 | Arduino Uno / Nano | Arduino Mega |
|:---|:---|:---|
| VCC | 5V | 5V |
| GND | GND | GND |
| DI | D9 | D19 (TX1) |
| RO | D8 | D18 (RX1) |
| DE | D7 | D22 |
| RE | D7 (jumper to DE) | D22 |

## Wiring: connect to STM32 Nucleo

| MAX485 | Nucleo |
|:---|:---|
| DI | **D8** |
| RO | **D2** |
| DE | **A2** |
| VCC | **3.3V** |
| GND | **GND** |

Bus: **A↔A**, **B↔B**, **Arduino GND ↔ Nucleo GND**.

---

## Run with STM32

1. `./run_rs485_arduino.sh uno` — flash Arduino, keep monitor open  
2. `./run_rs485.sh` — flash Nucleo (other terminal)

RS-485 bus: **38400 baud** (see `../rs485_bench_config.h`). USB serial monitors: **115200**.
