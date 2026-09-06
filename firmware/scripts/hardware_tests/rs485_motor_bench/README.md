# RS-485 Parallel Motor Bench

Control 4 stepper motors in parallel over RS-485: Pico master → 4× ESP32-C3 slaves → CL57T/CL42T drivers.

## Motor map

| Node ID | Driver | Motor |
|:---:|:---|:---|
| 1 | CL57T | NEMA 23 (J1) |
| 2 | CL57T | NEMA 23 (J2) |
| 3 | CL42T | NEMA 17 (J3) |
| 4 | CL42T | NEMA 17 (J4) |

## Wiring (ESP32-C3 SuperMini)

| ESP GPIO | Driver |
|:---:|:---|
| 5 | PUL+ |
| 6 | DIR+ |
| 20 | RS-485 TX (DI) |
| 21 | RS-485 RX (RO) |

ENA is hardwired enabled on the drivers. RS-485 DE+RE tied to GND on ESP nodes (RX-only).

Pico RS-485: GPIO0=TX, GPIO1=RX, GPIO2=DE (held HIGH, TX-only).

**Local J0 (no ESP):** board silk **pin 21 = GP16 → PUL+/STEP**, **pin 22 = GP17 → DIR+**.
Address as `0` or `j0` in the same commands (`run` / `jog` / `hold` / `wave` / `ping`).
`all` is still RS-485 broadcast only; `stop` halts bus nodes and local J0.

## Setup

1. Copy WiFi config:
   ```bash
   cp esp32/src/wifi_config.h.example esp32/src/wifi_config.h
   # Edit WIFI_SSID, WIFI_PASS, LOG_HOST_IP
   ```

2. Flash ESP32 nodes (one at a time):
   ```bash
   ./flash_motor_nodes.sh --node 1
   # ... through node 4
   ```

3. Flash Pico motor master:
   ```bash
   pio run -d pico -e pico -t upload
   ```

4. Start log hub (from this directory):
   ```bash
   ./run_motor_bench.sh
   ```

## Commands (log hub or Pico serial)

| Command | Action |
|:---|:---|
| `ping all` | Comms check — all nodes should log RX |
| `ping 0` / `ping j0` | Local Pico motor path (no bus) |
| `run 0 100 1 50` | **J0 (Pico GP16/GP17):** 100 steps forward @ 50 steps/s |
| `run j0 100 1 50` | Same as `run 0 …` |
| `run 1 100 1 50` | Node 1 only: 100 steps forward @ 50 steps/s |
| `run all 3200 1 500` | All RS-485 motors: 3200 steps forward @ 500 steps/s |
| `stop` | Emergency stop bus + local J0 |
| `jog 0 1 200` | J0 jog forward @ 200 steps/s until stop |
| `jog all 1 200` | All RS-485 motors jog forward @ 200 steps/s until stop |
| `bench` | Scripted test: creep → 1 revolution fwd/rev (RS-485) |
| `wave` / `wave all` | All RS-485 motors: 1000 steps @ 500 Hz, dir 1 then 0, forever |
| `wave 0` / `wave j0` | Local J0 wave (same defaults) |
| `wave 2` | Node 2 only (same defaults) |
| `wave 2 1000 500` | Node 2: 1000 steps @ 500 Hz each way |
| `wave all 2000 300` | All RS-485 motors: 2000 steps @ 300 Hz |

Direction flag: **`1` = forward, `0` = reverse** (not 2). `wave` always alternates 1 ↔ 0.

## ID filtering check

Before full-speed tests:

1. `ping all` — all 4 nodes respond in WiFi logs
2. `run 1 100 1 50` — only J1 logs `MOTOR|RUN`; nodes 2–4 increment `ignored`
3. `run all 100 1 50` — all 4 log `MOTOR|RUN` and move together

## Protocol

Bus frames use the shared ASCII format from `rs485_bench_protocol.h`:

```
@<dst> <cmd>\r\n
```

Motor commands (`rs485_motor_protocol.h`):

- `RUN <steps> <dir> <hz>` — `dir` 0 or 1, `hz` in steps/s
- `STOP` — halt immediately
- `JOG <dir> <hz>` — continuous until STOP

Broadcast destination `*` runs all nodes in parallel.

## Speed limits

Compile-time caps in `esp32/platformio.ini`:

- Nodes 1–2 (CL57T): 3000 steps/s max
- Nodes 3–4 (CL42T): 2000 steps/s max

## Related

- Comms-only bench (no motor GPIO): `../rs485_esp32_bench/`
- Log hub: `../rs485_log_hub.py`
- CL57T pulse patterns reference: `../cl57t_bench_arduino/`
