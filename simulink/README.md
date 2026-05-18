# Simulink Workspace

Model-based design and embedded code generation for the 6-DOF arm. This workspace is the **control-design and codegen** counterpart to the Python/Gazebo workspace under `/simulation/`.

For the full workflow rationale, conventions, and toolbox requirements, see [`../docs/implementation/simulink_workflow.md`](../docs/implementation/simulink_workflow.md).

---

## Role in the Project

Simulink has three jobs here:

1. **Plant modeling** (`plant_models/`) — Simscape Multibody model of a joint built from the project URDF. Captures motor electrical dynamics, cycloidal reduction (with backlash + friction), and link inertia.
2. **Controller design** (`controllers/`) — Per-joint PID (and optional feed-forward / gravity-comp) designed and tuned against the plant model using PID Tuner. Output is a controller subsystem block ready for code generation.
3. **Embedded code generation** (`codegen/`) — Embedded Coder generates C source for the controller, which is linked into the PlatformIO STM32 firmware build under `firmware/stm32_core/lib/control/`.

A stretch goal is **co-simulation with ROS 2** via ROS Toolbox: Simulink plant model subscribes to `/joint_commands` from MoveIt and publishes simulated `/joint_states` back, letting controller iteration happen without spinning up Gazebo.

---

## Layout

```
simulink/
├── README.md                     # ← this file
├── plant_models/                 # Simscape Multibody / transfer-function plants
│   └── README.md
├── controllers/                  # PID + feed-forward controller designs (.slx)
│   └── README.md
├── codegen/                      # Embedded Coder configuration and generated C output
│   └── README.md
├── scripts/                      # MATLAB .m setup scripts (workspace init, param loading)
│   └── README.md
└── tests/                        # Test harnesses (unit + integration test models)
    └── README.md
```

The actual `.slx` model files are authored in MATLAB — this directory is the agreed home for them. Generated C under `codegen/` is checked in so the firmware build does not require a MATLAB license.

---

## Requirements

- **MATLAB R2024a or newer**
- **Simulink** (block-diagram environment)
- **Simscape** + **Simscape Multibody** (plant modeling)
- **Control System Toolbox** (PID Tuner)
- **Embedded Coder** + **Simulink Coder** (C code generation for STM32)
- **MATLAB Coder** (transitive dependency of Embedded Coder)
- *(optional)* **ROS Toolbox** — only required for the ROS 2 co-simulation stretch goal

Toolbox licensing: campus / educational MATLAB licenses typically include Simscape, Control System, and Embedded Coder. If running on a personal home license, verify Embedded Coder is included before relying on the codegen path.

---

## Onboarding

1. Open MATLAB and `cd` to this directory.
2. Run `scripts/setup_workspace.m` to load joint parameters (motor specs, gear ratios, link inertias) from the project constants into the MATLAB base workspace.
3. Open the plant model under `plant_models/joint_J1_plant.slx`.
4. Open the corresponding controller `controllers/joint_J1_pid.slx` and run PID Tuner to refine gains.
5. To regenerate C code, open `codegen/joint_pid_codegen.slx`, run `Build` (Ctrl+B); the generated source appears under `codegen/output/` and is committed.

---

## Integration with the firmware build

Generated C from Embedded Coder is dropped into `firmware/stm32_core/lib/control/generated/` and wrapped by a hand-written adapter (`joint_controller_generated.cpp`) that bridges the generated function signatures to the rest of the firmware. See [`../docs/implementation/firmware_architecture.md §2b`](../docs/implementation/firmware_architecture.md) for the integration contract.

The firmware build does **not** require MATLAB — only the regeneration of controller code does. This keeps the project buildable for collaborators without MATLAB licenses.
