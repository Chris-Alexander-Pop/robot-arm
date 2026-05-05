#!/usr/bin/env python3
"""Emit robot_arm KiCad 10 schematics (minimal draft). Run kicad-cli sch upgrade afterward."""

from __future__ import annotations

import uuid
from pathlib import Path


def u() -> str:
    return str(uuid.uuid4())


def effects(size: float = 1.27) -> str:
    return f"(effects (font (size {size} {size})) (justify left bottom))"


def global_label(name: str, shape: str, x: float, y: float, angle: float = 0.0) -> str:
    return f""" (global_label "{name}"
    (shape {shape})
    (at {x} {y} {angle})
    {effects()}
    (uuid "{u()}")
  )"""


def graphic_text(text: str, x: float, y: float, w: float = 12.0) -> str:
    # KiCad expects literal \\n inside quoted strings for line breaks.
    esc = text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    return f""" (text "{esc}"
    (at {x} {y} 0)
    {effects(1.5)}
    (uuid "{u()}")
  )"""


def kicad_sch(body: str, title: str, rev: str = "0.1", root_uuid: str | None = None) -> str:
    rid = root_uuid or u()
    return f"""(kicad_sch
  (version 20231120)
  (generator "robot_arm_tools.generate_schematics")
  (generator_version "1.0")
  (uuid "{rid}")
  (paper "A3")
  (title_block
    (title "{title}")
    (date "2026-05-04")
    (rev "{rev}")
  )
  (lib_symbols)
{body}
  (sheet_instances
    (path "/" (page "1"))
  )
  (embedded_fonts no)
)
"""


def write_power_entry() -> str:
    notes = """Power entry (implements docs/implementation/electrical_schematic_plan.md Sec.2a)
- IEC C14 + fuse, E-stop NC in series with AC feed
- Mean Well LRS-350-24 -> +24V_MOTOR / GND_MOTOR
- Bond chassis / FG per PSU manual"""
    body = f"""
  {graphic_text(notes, 15, 15, 180)}
  {global_label("24V_MOTOR", "bidirectional", 200, 120, 0)}
  {global_label("GND_MOTOR", "bidirectional", 200, 135, 0)}
"""
    return kicad_sch(body, "Power entry")


def write_logic_power() -> str:
    notes = """Logic power (sheet Sec.2b)
- LM2596 #1: 24V -> 5V_LOGIC (STM32, 74HC245, Hall, TCA9548A)
- LM2596 #2: 24V -> 5V_PI (Raspberry Pi 3)
- LM2596 #3: 24V -> 5V_SERVO (MG996R only)
- Star GND at PSU return; keep Pi/servo bucks separate for noise"""
    body = f"""
  {graphic_text(notes, 15, 15)}
  {global_label("24V_MOTOR", "bidirectional", 40, 160, 0)}
  {global_label("5V_LOGIC", "bidirectional", 200, 100, 0)}
  {global_label("5V_PI", "bidirectional", 200, 115, 0)}
  {global_label("5V_SERVO", "bidirectional", 200, 130, 0)}
  {global_label("GND_LOGIC", "bidirectional", 200, 160, 0)}
"""
    return kicad_sch(body, "Logic power")


def write_stm32_control() -> str:
    notes = """STM32 + Pi (sheet Sec.2c) - Nucleo-F401RE assumed (see docs/implementation/electrical_interface_map.md Sec.5)
- STEP/DIR/ENABLE J1-J4: 3.3V MCU -> 74HC245 -> 5V CL57T/CL42T (set drivers for 5V TTL inputs)
- STEP/DIR/ENABLE J5-J6: 3.3V MCU -> TMC2209 carriers (module pinout varies - verify Amazon breakout)
- UART: Pi <-> STM32 (e.g. USART1 PA9/PA10 candidates - confirm on Nucleo)
- I2C1: PB6/PB7 -> TCA9548A -> optional AS5600 on J5/J6
- Six DRV_ALARM inputs (active-low); GRIPPER_PWM -> servo signal"""
    lines = [graphic_text(notes, 12, 12)]
    y_step, y_dir, y_en = 88.0, 103.0, 118.0
    for j in range(1, 7):
        x = 22 + j * 24
        lines.append(global_label(f"STM32_STEP_J{j}", "bidirectional", x, y_step, 0))
        lines.append(global_label(f"STM32_DIR_J{j}", "bidirectional", x, y_dir, 0))
        lines.append(global_label(f"STM32_EN_J{j}", "bidirectional", x, y_en, 0))
    # EN uses STM32_EN_J*
    for j in range(1, 7):
        lines.append(global_label(f"DRV_ALARM_J{j}", "bidirectional", 25 + j * 22, 150, 0))
        lines.append(global_label(f"HOME_J{j}", "bidirectional", 25 + j * 22, 175, 0))
    lines += [
        global_label("UART_TX_TO_STM32", "bidirectional", 220, 95, 0),
        global_label("UART_RX_FROM_STM32", "bidirectional", 220, 110, 0),
        global_label("I2C_SCL", "bidirectional", 220, 130, 0),
        global_label("I2C_SDA", "bidirectional", 220, 145, 0),
        global_label("GRIPPER_PWM", "bidirectional", 220, 165, 0),
        global_label("5V_SERVO", "bidirectional", 220, 180, 0),
        global_label("5V_LOGIC", "bidirectional", 220, 195, 0),
        global_label("GND_LOGIC", "bidirectional", 220, 210, 0),
    ]
    body = "\n  " + "\n  ".join(lines)
    return kicad_sch(body, "STM32 control + level shifting")


def write_motor_joint(j: int, driver: str, iclass: str) -> str:
    notes = f"""Joint J{j} - {driver}
Closed-loop (CL): motor phases + encoder harness from kit; logic PUL/DIR/ENA (5V) + ALARM -> DRV_ALARM_J{j}
TMC2209: VM=24V_MOTOR, GND, STEP/DIR/EN from MCU (3.3V); verify MS/DIAG pins on YOUR carrier.

Netclass hint: {iclass}"""
    body = f"""
  {graphic_text(notes, 12, 12)}
  {global_label(f"STM32_STEP_J{j}", "bidirectional", 180, 70, 0)}
  {global_label(f"STM32_DIR_J{j}", "bidirectional", 180, 85, 0)}
  {global_label(f"STM32_EN_J{j}", "bidirectional", 180, 100, 0)}
  {global_label(f"DRV_ALARM_J{j}", "bidirectional", 180, 120, 0)}
  {global_label("24V_MOTOR", "bidirectional", 180, 145, 0)}
  {global_label("GND_MOTOR", "bidirectional", 180, 165, 0)}
"""
    return kicad_sch(body, f"Motor driver J{j}")


def write_sensors() -> str:
    notes = """Sensors (sheet Sec.2e)
- TCA9548A @ 0x70: upstream I2C_SCL/SDA -> downstream channels for AS5600 (fixed 0x36 each)
- A3144 Hall: open-collector -> HOME_Jx with pull-up on MCU board (5V_LOGIC / 3.3V per chosen divider)
- Optional AS5600 DNP on J5/J6 for phase 1"""
    body = f"""
  {graphic_text(notes, 12, 12)}
  {global_label("I2C_SCL", "bidirectional", 160, 90, 0)}
  {global_label("I2C_SDA", "bidirectional", 160, 105, 0)}
  {global_label("5V_LOGIC", "bidirectional", 160, 130, 0)}
  {global_label("GND_LOGIC", "bidirectional", 160, 145, 0)}
"""
    for j in range(1, 7):
        body += f"""
  {global_label(f"HOME_J{j}", "bidirectional", 280, 70 + j * 18, 0)}
"""
    return kicad_sch(body, "Sensors & homing")


def write_root(_out_dir: Path) -> str:
    """Flat index sheet: KiCad 10 rejects minimal third-party hierarchical `sheet` tokens, so subsheets are separate files. Use Place hierarchical sheet in Eeschema to embed them, or open each file as a tab."""

    root_uuid = u()
    lines = [
        "Robot arm electrical index (draft).",
        "Global net names match docs/implementation/electrical_schematic_plan.md",
        "",
        "Subsheets (open alongside this project):",
        "- power_entry.kicad_sch",
        "- logic_power.kicad_sch",
        "- stm32_control.kicad_sch",
        "- sensors.kicad_sch",
        "- motor_joint_J1.kicad_sch .. motor_joint_J6.kicad_sch",
        "",
        "Next step in KiCad: Place > Hierarchical Sheet for each file above to build the tree.",
    ]
    blocks = [graphic_text("\n".join(lines), 12, 18)]
    body = "\n  " + "\n  ".join(blocks)
    return kicad_sch(body, "Robot arm - root index", rev="0.1", root_uuid=root_uuid)


def main() -> None:
    root = Path(__file__).resolve().parent.parent
    motor_specs = [
        (1, "CL57T closed-loop kit", "MotorPower"),
        (2, "CL57T closed-loop kit", "MotorPower"),
        (3, "CL42T closed-loop kit", "MotorPower"),
        (4, "CL42T closed-loop kit", "MotorPower"),
        (5, "TMC2209 carrier + NEMA14", "MotorPower"),
        (6, "TMC2209 carrier + NEMA14", "MotorPower"),
    ]
    files = {
        "power_entry.kicad_sch": write_power_entry(),
        "logic_power.kicad_sch": write_logic_power(),
        "stm32_control.kicad_sch": write_stm32_control(),
        "sensors.kicad_sch": write_sensors(),
        "robot_arm.kicad_sch": "",  # filled below
    }
    for j, drv, icls in motor_specs:
        files[f"motor_joint_J{j}.kicad_sch"] = write_motor_joint(j, drv, icls)
    files["robot_arm.kicad_sch"] = write_root(root)

    for name, content in files.items():
        path = root / name
        path.write_text(content, encoding="utf-8")
        print("wrote", path)


if __name__ == "__main__":
    main()
