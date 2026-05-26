# RS-485 bench test — quick start (PlatformIO)

Two boards, two MAX485 modules, **38400 baud** on the RS-485 bus (USB monitors stay 115200). No Arduino IDE required.

## 1. Wire

| | Arduino Uno/Nano | Nucleo-F401RE |
|:---|:---|:---|
| MAX485 VCC | 5V (or 3.3V if 3.3V module) | **3.3V** |
| MAX485 GND | GND | GND |
| DI | **D9** | **D8** |
| RO | **D8** | **D2** |
| DE | **D7** | **A2** |

**Between modules:** A↔A, B↔B, **Arduino GND ↔ Nucleo GND** (required).

## 2. Flash Arduino (PlatformIO)

From the repo:

```sh
cd firmware/scripts/hardware_tests
chmod +x run_rs485_arduino.sh   # once
./run_rs485_arduino.sh uno      # or: nano | mega
```

Or manually:

```sh
cd firmware/scripts/hardware_tests/rs485_arduino
pio run -e uno -t upload
pio device monitor -e uno
```

**VS Code / Cursor:** open folder `firmware/scripts/hardware_tests/rs485_arduino` as the PlatformIO project, pick env `uno` / `nano` / `mega`, Upload + Monitor.

Leave the monitor open (or note the COM port) — you should see the banner and `Ready — waiting for RS485 PING...`.

## 3. Flash STM32 (second USB port / cable)

In another terminal:

```sh
cd firmware/scripts/hardware_tests
./run_rs485.sh
```

## 4. Pass criteria

**STM32 monitor:** every ~1 s, `TX: RS485 PING` and `RX (12 OK)` when echo is clean.

**Arduino monitor:** `RX (12 OK)` and `TX echo` — `bad=` should stay near zero.

For a long harness later: add **120 Ω** between A and B at each bus end; keep **GND** reference.

## 5. Troubleshooting

- Common **GND** Arduino ↔ Nucleo
- Swap **A** and **B** on one module
- Wrong board env: `pio run -e nano` for Arduino Nano, etc.
- List ports: `pio device list`
