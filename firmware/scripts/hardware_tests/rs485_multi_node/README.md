# RS-485 multi-node bench: Pico master + 5 ESP32-C3 nodes

Full wiring, flash, and operation guide for the bench stack in this directory.

---

## Hardware you need

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Raspberry Pi Pico | USB-B micro to PC |
| 5 | ESP32-C3 module (devkitm-1 or similar) | USB-C to PC for flashing; WiFi for logs |
| 6 | MAX485 or MAX3485 module | 3.3V-compatible on Pico/ESP32-C3 |
| 1 | Bench WiFi AP or router | PC and all ESP32-C3s on the same LAN |
| — | Dupont wires, breadboard | See wiring table below |

---

## Bus topology

```text
PC ─USB─ Pico ─UART0─ MAX485 ──A/B/GND──► ESP32-C3 #1
                                        ──► ESP32-C3 #2
                                        ──► ESP32-C3 #3
                                        ──► ESP32-C3 #4
                                        ──► ESP32-C3 #5
                                    (daisy-chained A/B/GND)

PC ◄─── WiFi UDP port 9000 ─── ESP32-C3 #1..#5
```

All nodes share the same RS-485 pair (A, B) and a common GND.  The Pico sends
addressed frames; only the targeted node replies on the bus.  Every node also
logs to the PC over WiFi so you can watch all five simultaneously.

---

## Wiring

### Pico → MAX485

| MAX485 pin | Pico GPIO | Notes |
|------------|-----------|-------|
| VCC        | 3V3       | |
| GND        | GND       | |
| DI         | GPIO 0    | UART0 TX |
| RO         | GPIO 1    | UART0 RX |
| DE + RE    | GPIO 2    | tie DE and RE together |
| A, B       | Bus       | to daisy-chain |

### ESP32-C3 → MAX485 (×5, same wiring per node)

| MAX485 pin | ESP32-C3 GPIO | Notes |
|------------|---------------|-------|
| VCC        | 3V3           | |
| GND        | GND           | |
| DI         | GPIO 4        | UART1 TX |
| RO         | GPIO 5        | UART1 RX |
| DE + RE    | GPIO 6        | tie DE and RE together |
| A, B       | Bus           | daisy-chain from previous node |

> **If your carrier board uses different RS-485 pins**, edit  
> `rs485_esp32_bench/src/pinout.h` and `rs485_pico/src/pinout.h` before
> building. No other files need to change.

### Bus connections

```text
Pico MAX485  A ──────────────────────────────────────────────────── A  all nodes
Pico MAX485  B ──────────────────────────────────────────────────── B  all nodes
Pico GND       ──────────────────────────────────────────────────── GND all nodes
```

Minimum: **3 wires** across all boards (A, B, GND). GND must be common to all.

Optional: 120 Ω between A and B at the **Pico end** and the **last ESP32 end**.
On a short bench run (<30 cm) you can skip termination; add it if you see rising
`bad=` counts.

---

## WiFi setup

1. Copy the template:
   ```sh
   cp rs485_esp32_bench/src/wifi_config.h.example rs485_esp32_bench/src/wifi_config.h
   ```
2. Edit `wifi_config.h` and set `WIFI_SSID`, `WIFI_PASS`, and `LOG_HOST_IP`
   (your PC's LAN IP on the bench network).
3. Make sure the PC running the hub and all ESP32-C3 boards are on the **same LAN**.
4. The UDP port is **9000** by default; change `LOG_UDP_PORT` in both
   `wifi_config.h` and `platformio.ini` if you need a different port.

---

## Flash order

### Step 1 — flash ESP32-C3 nodes

Interactive (prompts per node, one USB at a time):
```sh
cd firmware/scripts/hardware_tests
./flash_rs485_esp32_nodes.sh
```

Or flash a single node:
```sh
./flash_rs485_esp32_nodes.sh --node 3
```

After each flash, open the USB serial monitor to confirm the boot banner:
```sh
pio device monitor -d rs485_esp32_bench -e node_3
```
Expected output:
```
=========================================
 RS-485 bench slave — node id=3
=========================================
  Bus baud : 38400
  UART1: GPIO4=TX, GPIO5=RX, GPIO6=DE
[wifi] connecting to MyNet .........
[wifi] connected, IP=192.168.1.42
[wifi] log hub: 192.168.1.100:9000
```

### Step 2 — flash Pico master

```sh
./run_rs485_pico.sh
```

The script builds, flashes, and opens the monitor. You should see:
```
=========================================
 RS-485 multi-node bench — Pico master
=========================================
  Bus baud : 38400
  Type 'help' for commands.
```

### Step 3 — start the log hub (PC)

In a separate terminal:
```sh
./run_rs485_log_hub.sh
```

Or with an explicit Pico port:
```sh
./run_rs485_log_hub.sh --pico-port /dev/ttyACM0
```

---

## Usage

Type at the hub prompt (`>`):

| Command | Action |
|---------|--------|
| `1` .. `5` | Ping one node — only that node's WiFi log line should appear |
| `all` | Broadcast ping — all five nodes respond |
| `ping 2` | Same as `2` |
| `ping all` | Same as `all` |
| `auto 2000` | Start auto-sweep every 2000 ms (cycles through nodes 1→5) |
| `stop` | Stop auto-sweep |
| `status` | Print per-node ok/bad/ignored counters |
| `help` | Full command list |
| `q` | Quit |

### Expected output when you type `2`

```
> 2
15:04:22 [J2|RX] @2 PING ok=1 bad=0 ignored=8
```

Nodes J1/J3/J4/J5 should show **no** new log line (they silently increment
their `ignored` counter, visible via `status`).

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| `bad=` rising on a node | A/B polarity swapped | Swap A and B wires on **one** module |
| All nodes answer every ping | Common GND missing | Add GND between all transceivers |
| Wrong node answers | Build flag mismatch | Check `ROBOT_ARM_NODE_ID` per env in `platformio.ini` |
| No WiFi `BOOT` line in hub | IP or SSID wrong | Confirm `wifi_config.h`; check AP is up |
| `[hub] no Pico serial port found` | Pico not enumerated | Check USB cable is data-capable; try `ls /dev/ttyACM*` |
| Node ID 0 on boot | Not configured | Run `NODE_ID 3` in the USB serial monitor |
| Pico sends but gets no reply | Bus wiring or baud | Confirm 38400 on both sides; check DE polarity |

---

## File map

```
firmware/scripts/hardware_tests/
├── rs485_bench_config.h          shared baud / timing constants (38400)
├── rs485_bench_protocol.h        addressed @dst framing + parse helpers
├── rs485_pico/                   Pico master PlatformIO project
│   ├── platformio.ini
│   └── src/
│       ├── main.cpp              command shell + ping TX/RX
│       └── pinout.h              GPIO0=TX, GPIO1=RX, GPIO2=DE
├── rs485_esp32_bench/            ESP32-C3 slave PlatformIO project
│   ├── platformio.ini            envs node_1..node_5
│   └── src/
│       ├── main.cpp              RS485 RX, ACK, counters
│       ├── pinout.h              GPIO4=TX, GPIO5=RX, GPIO6=DE
│       ├── wifi_log.h/cpp        WiFi UDP log shipper
│       └── wifi_config.h.example → copy to wifi_config.h and fill in
├── rs485_log_hub.py              Python hub: UDP merger + Pico serial
├── rs485_bench.env.example       → copy to rs485_bench.env for credentials
├── run_rs485_pico.sh             build + flash + monitor the Pico
├── flash_rs485_esp32_nodes.sh    batch-flash all five ESP32-C3 nodes
└── run_rs485_log_hub.sh          start the Python hub
```
