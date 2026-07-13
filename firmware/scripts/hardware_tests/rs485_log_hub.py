#!/usr/bin/env python3
"""
rs485_log_hub.py — Multi-node RS-485 bench log aggregator.

Receives UDP datagrams from up to 5 ESP32-C3 nodes (each running
rs485_esp32_bench firmware) and forwards commands to the Pico master
over its USB serial port.  All output is merged to stdout with timestamps
and per-node colour so you can watch all nodes at once.

Usage:
    python3 rs485_log_hub.py [--pico-port /dev/ttyACM0] [--udp-port 9000]

Interactive commands (type at the prompt):
    1 .. 5          send "ping <n>" to the Pico (targeted ping)
    all             broadcast ping (@*)
    ping <n|all>    explicit form of the above
    run all <steps> <dir> <hz>   broadcast motor RUN
    run <n> <steps> <dir> <hz>    unicast motor RUN
    jog all <dir> <hz>            broadcast motor JOG
    bench           scripted parallel motor test (motor bench Pico firmware)
    wave [all|1-4] [steps] [hz]  continuous fwd/rev (default all 1000 @ 500; dir 1=fwd 0=rev)
    auto [ms]       start auto-sweep (default 2000 ms between pings)
    stop            motor STOP + stop auto-sweep
    status          print per-node counters + motor state
    help            print this list
    q / quit        exit

Dependencies (stdlib only — no pip installs needed):
    socket, threading, select, time, sys, os, argparse, re
Optional (auto-detected):
    pyserial        for Pico serial communication
                    install: pip install pyserial
"""

import argparse
import os
import queue
import re
import socket
import sys
import threading
import time

# ── ANSI colours ──────────────────────────────────────────────────────────────

_RESET  = "\033[0m"
_BOLD   = "\033[1m"
_DIM    = "\033[2m"
_RED    = "\033[91m"
_GREEN  = "\033[92m"
_YELLOW = "\033[93m"
_BLUE   = "\033[94m"
_MAGENTA= "\033[95m"
_CYAN   = "\033[96m"

# One colour per node ID 1..5
_NODE_COLOURS = {
    1: _GREEN,
    2: _CYAN,
    3: _YELLOW,
    4: _MAGENTA,
    5: _BLUE,
}

def _node_colour(node_id: int) -> str:
    return _NODE_COLOURS.get(node_id, _RESET)

def _ts() -> str:
    return time.strftime("%H:%M:%S")

def _print_log(prefix: str, msg: str, colour: str = _RESET) -> None:
    # One line per event; do not re-print the input prompt here (that caused
    # "> > > ..." spam when UDP/Pico traffic was high).
    print(f"{colour}{_BOLD}{_ts()} {prefix}{_RESET} {msg}", flush=True)

# ── Per-node statistics ───────────────────────────────────────────────────────

class NodeStats:
    def __init__(self, node_id: int):
        self.node_id   = node_id
        self.seen      = False
        self.last_seen = 0.0
        self.ok        = 0
        self.bad       = 0
        self.ignored   = 0
        self.boot_time = 0.0
        self.motor_state = "idle"
        self.motor_detail = ""

    def update_from_log(self, tag: str, msg: str) -> None:
        self.seen      = True
        self.last_seen = time.time()
        # Parse counter values embedded in RX lines: ok=12 bad=0 ignored=40
        m_ok  = re.search(r'ok=(\d+)',      msg)
        m_bad = re.search(r'bad=(\d+)',     msg)
        m_ign = re.search(r'ignored=(\d+)', msg)
        if m_ok:  self.ok      = int(m_ok.group(1))
        if m_bad: self.bad     = int(m_bad.group(1))
        if m_ign: self.ignored = int(m_ign.group(1))
        if tag == "BOOT":
            self.boot_time = time.time()
        if tag == "MOTOR":
            if msg.startswith("RUN start") or msg.startswith("JOG start"):
                self.motor_state = "running"
                self.motor_detail = msg
            elif msg.startswith("done"):
                self.motor_state = "done"
                self.motor_detail = msg
            elif msg.startswith("STOP"):
                self.motor_state = "stopped"
                self.motor_detail = msg

_stats: dict[int, NodeStats] = {i: NodeStats(i) for i in range(1, 6)}

# ── UDP listener thread ────────────────────────────────────────────────────────

def _udp_listener(udp_port: int, out_queue: "queue.Queue[tuple[int,str,str]]") -> None:
    """Receive UDP datagrams and push (node_id, tag, msg) to out_queue."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", udp_port))
    sock.settimeout(1.0)
    while True:
        try:
            data, _addr = sock.recvfrom(4096)
        except socket.timeout:
            continue
        line = data.decode("utf-8", errors="replace").strip()
        # Nodes may batch several log lines in one datagram (newline-separated).
        for entry in line.splitlines():
            entry = entry.strip()
            if not entry:
                continue
            # Expected format: J<id>|<tag>|<msg>
            parts = entry.split("|", 2)
            if len(parts) < 3 or not parts[0].startswith("J"):
                continue
            try:
                node_id = int(parts[0][1:])
            except ValueError:
                continue
            tag = parts[1]
            msg = parts[2] if len(parts) > 2 else ""
            out_queue.put((node_id, tag, msg))

# ── Pico serial thread ────────────────────────────────────────────────────────

def _pico_reader(ser, out_queue: "queue.Queue[tuple[int,str,str]]") -> None:
    """Read lines from Pico serial and push as (0, 'PICO', line)."""
    while True:
        try:
            raw = ser.readline()
        except Exception:
            break
        if not raw:
            continue
        line = raw.decode("utf-8", errors="replace").strip()
        if line:
            out_queue.put((0, "PICO", line))

# ── Auto-sweep state ──────────────────────────────────────────────────────────

class AutoSweep:
    def __init__(self):
        self.active    = False
        self.interval  = 2.0   # seconds
        self._next_id  = 1
        self._last     = 0.0

    def tick(self) -> "str | None":
        """Return a command string to send, or None if not time yet."""
        if not self.active:
            return None
        now = time.time()
        if now - self._last < self.interval:
            return None
        self._last = now
        cmd = f"ping {self._next_id}"
        self._next_id = (self._next_id % 5) + 1
        return cmd

# ── Command dispatch ──────────────────────────────────────────────────────────

def _send_to_pico(ser, cmd: str) -> None:
    """Write a command line to the Pico serial port."""
    line = cmd.strip() + "\r\n"
    ser.write(line.encode("utf-8"))
    ser.flush()

def _handle_user_command(raw: str, ser, sweep: AutoSweep) -> bool:
    """
    Parse and act on a user command.
    Returns False if the user wants to quit.
    """
    cmd = raw.strip().lower()

    if cmd in ("q", "quit", "exit"):
        return False

    if cmd == "help":
        print(
            "Commands:\n"
            "  1..5          ping one node\n"
            "  all           broadcast ping\n"
            "  ping <n|all>  explicit ping\n"
            "  run all <steps> <dir> <hz>   broadcast motor RUN\n"
            "  run <n> <steps> <dir> <hz>    unicast motor RUN\n"
            "  jog all <dir> <hz>            broadcast motor JOG\n"
            "  jog <n> <dir> <hz>            unicast motor JOG\n"
            "  bench                         scripted parallel motor test\n"
            "  wave [all|1-4] [steps] [hz]   back/forth (def all 1000 @ 500; dir 1/0)\n"
            "  auto [ms]     start auto-sweep\n"
            "  stop          stop motors + auto-sweep\n"
            "  status        per-node counters + motor state\n"
            "  help          this list\n"
            "  q / quit      exit"
        )
        return True

    if cmd == "status":
        print(f"\r{'Node':>5}  {'seen':>6}  {'ok':>6}  {'bad':>6}  "
              f"{'ignored':>8}  {'motor':>8}  last_seen")
        for nid in range(1, 6):
            st = _stats[nid]
            age = f"{time.time() - st.last_seen:.1f}s ago" if st.seen else "never"
            colour = _node_colour(nid)
            print(f"{colour}  J{nid}  {str(st.seen):>6}  {st.ok:>6}  "
                  f"{st.bad:>6}  {st.ignored:>8}  {st.motor_state:>8}  {age}{_RESET}")
        return True

    if cmd in ("1","2","3","4","5"):
        if ser:
            _send_to_pico(ser, f"ping {cmd}")
        else:
            print("[hub] no Pico connected — cannot send command")
        return True

    if cmd == "all":
        if ser:
            _send_to_pico(ser, "ping all")
        else:
            print("[hub] no Pico connected")
        return True

    if cmd.startswith("ping "):
        arg = cmd[5:].strip()
        if ser:
            _send_to_pico(ser, f"ping {arg}")
        else:
            print("[hub] no Pico connected")
        return True

    if cmd.startswith("run "):
        if ser:
            _send_to_pico(ser, cmd)
        else:
            print("[hub] no Pico connected")
        return True

    if cmd.startswith("jog "):
        if ser:
            _send_to_pico(ser, cmd)
        else:
            print("[hub] no Pico connected")
        return True

    if cmd.startswith("hold "):
        if ser:
            _send_to_pico(ser, cmd)
        else:
            print("[hub] no Pico connected")
        return True

    if cmd == "bench":
        if ser:
            _send_to_pico(ser, "bench")
        else:
            print("[hub] no Pico connected")
        return True

    if cmd == "wave" or cmd.startswith("wave "):
        if ser:
            _send_to_pico(ser, cmd)
        else:
            print("[hub] no Pico connected")
        return True

    if cmd.startswith("auto"):
        rest = cmd[4:].strip()
        if rest:
            try:
                ms = int(rest)
                if ms >= 100:
                    sweep.interval = ms / 1000.0
            except ValueError:
                pass
        sweep.active = True
        sweep._last = 0.0
        print(f"[hub] auto-sweep started, interval={sweep.interval*1000:.0f} ms")
        return True

    if cmd == "stop":
        sweep.active = False
        if ser:
            _send_to_pico(ser, "stop")
            print("[hub] motor STOP sent; auto-sweep stopped")
        else:
            print("[hub] auto-sweep stopped (no Pico — cannot send motor STOP)")
        return True

    if cmd:
        print(f"[hub] unknown command: {cmd!r}  (type 'help')")
    return True

# ── Main ──────────────────────────────────────────────────────────────────────

def _find_pico_port() -> "str | None":
    """Return Pico USB serial port; skip Espressif ESP32-C3 CDC devices."""
    import glob

    for pattern in (
        "/dev/serial/by-id/*Pico*",
        "/dev/serial/by-id/*RP2040*",
        "/dev/serial/by-id/*Raspberry*",
    ):
        for by_id in sorted(glob.glob(pattern)):
            try:
                return os.path.realpath(by_id)
            except OSError:
                continue

    esp_ports: set[str] = set()
    for by_id in glob.glob("/dev/serial/by-id/usb-Espressif_*"):
        try:
            esp_ports.add(os.path.realpath(by_id))
        except OSError:
            continue

    for i in range(8):
        p = f"/dev/ttyACM{i}"
        if not os.path.exists(p):
            continue
        try:
            if os.path.realpath(p) in esp_ports:
                continue
        except OSError:
            pass
        return p
    return None


def main() -> None:
    parser = argparse.ArgumentParser(description="RS-485 multi-node bench log hub")
    parser.add_argument("--pico-port", default=None,
                        help="Serial port for the Pico master (e.g. /dev/ttyACM1). "
                             "Auto-detected if omitted.")
    parser.add_argument("--no-pico", action="store_true",
                        help="Do not open any serial port (UDP logs only)")
    parser.add_argument("--udp-port", type=int, default=9000,
                        help="UDP port to listen on for node logs (default: 9000)")
    parser.add_argument("--no-colour", action="store_true",
                        help="Disable ANSI colour output")
    args = parser.parse_args()

    if args.no_colour:
        for key in ("_RESET","_BOLD","_DIM","_RED","_GREEN","_YELLOW",
                    "_BLUE","_MAGENTA","_CYAN"):
            globals()[key] = ""
        for k in _NODE_COLOURS:
            _NODE_COLOURS[k] = ""

    # ── Serial ────────────────────────────────────────────────────────────────
    ser = None
    pico_port = None if args.no_pico else (args.pico_port or _find_pico_port())
    if pico_port:
        try:
            import serial as pyserial
            ser = pyserial.Serial(pico_port, 115200, timeout=0.1)
            print(f"[hub] Pico serial: {pico_port} @ 115200")
        except ImportError:
            print("[hub] pyserial not installed — serial commands unavailable")
            print("      pip install pyserial")
        except Exception as e:
            print(f"[hub] could not open {pico_port}: {e}")
    else:
        print("[hub] no Pico serial port found — UDP monitoring only")
        print("      use --pico-port /dev/ttyACMx if the Pico is connected")

    # ── UDP thread ─────────────────────────────────────────────────────────────
    event_queue: queue.Queue = queue.Queue()
    udp_thread = threading.Thread(
        target=_udp_listener, args=(args.udp_port, event_queue), daemon=True
    )
    udp_thread.start()
    print(f"[hub] listening for UDP logs on 0.0.0.0:{args.udp_port}")

    # ── Pico reader thread ─────────────────────────────────────────────────────
    if ser:
        pico_thread = threading.Thread(
            target=_pico_reader, args=(ser, event_queue), daemon=True
        )
        pico_thread.start()

    # ── Sweep ──────────────────────────────────────────────────────────────────
    sweep = AutoSweep()

    print()
    print("[hub] ready — type 'help' for commands, Ctrl-C or 'q' to quit")
    print()

    # Use non-blocking stdin reads so the event loop can still drain the queue
    import select

    interactive = sys.stdin.isatty()

    try:
        if interactive:
            print("> ", end="", flush=True)
        while True:
            # ── Drain UDP / Pico event queue ──────────────────────────────────
            try:
                while True:
                    node_id, tag, msg = event_queue.get_nowait()
                    if node_id == 0:
                        # Pico serial line
                        colour = _DIM
                        _print_log("[PICO]", msg, colour)
                    else:
                        colour = _node_colour(node_id)
                        st = _stats.get(node_id)
                        if st:
                            st.update_from_log(tag, msg)
                        _print_log(f"[J{node_id}|{tag}]", msg, colour)
            except queue.Empty:
                pass

            # ── Auto-sweep ────────────────────────────────────────────────────
            sweep_cmd = sweep.tick()
            if sweep_cmd and ser:
                _send_to_pico(ser, sweep_cmd)

            # ── Non-blocking stdin (interactive terminal only) ─────────────────
            if interactive:
                rlist, _, _ = select.select([sys.stdin], [], [], 0.05)
                if rlist:
                    raw = sys.stdin.readline()
                    if raw:
                        if not _handle_user_command(raw, ser, sweep):
                            break
                        print("> ", end="", flush=True)
            else:
                time.sleep(0.05)

    except KeyboardInterrupt:
        pass

    print("\n[hub] exiting")
    if ser:
        ser.close()


if __name__ == "__main__":
    main()
