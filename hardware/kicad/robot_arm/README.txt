Robot arm KiCad project (KiCad 10 compatible)

Open: robot_arm.kicad_pro

What is already in git (no GUI required):
  - Multi-page draft schematics: net names as global labels + wiring notes on power_entry, logic_power,
    stm32_control, sensors, motor_joint_J1..J6.
  - Python generator tools/generate_schematics.py and symbols/driver_interface_notes.txt.
  - kicad-cli can upgrade/export PDFs from these files.

What you should do once in Eeschema (minutes, not hours):
  - Place > Hierarchical Sheet on robot_arm.kicad_sch for each subsheet file so the tree matches
    docs/implementation/electrical_schematic_plan.md (KiCad 10 did not load hand-written sheet blocks).
  - Replace text blocks with real symbols from KiCad libraries (connectors, regulators, MCU header,
    driver blocks), draw wires, run ERC.

Regenerate drafts after editing the generator:
  python3 tools/generate_schematics.py && kicad-cli sch upgrade --force *.kicad_sch

Net classes MotorPower vs Logic are in robot_arm.kicad_pro for PCB routing later; assign remaining nets in the schematic as you refine it.

Driver terminal names (verify against YOUR vendor PDF revision):
  CL57T / CL42T (StepperOnline-style): PUL/DIR/ENA/ALRM + motor phases + DC power + encoder harness from kit.
  TMC2209 carrier (generic STEPStick-style): VM, GND, STEP, DIR, EN; MS1/MS2/VREF/DIAG vary by breakout.
