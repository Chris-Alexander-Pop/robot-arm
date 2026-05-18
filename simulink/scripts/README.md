# MATLAB Scripts

> **Planned — not yet in repository** — setup scripts are not checked in. See [`../../docs/implementation/simulink_workflow.md`](../../docs/implementation/simulink_workflow.md).

MATLAB `.m` helpers for the Simulink workspace.

## Planned scripts

| File | Purpose |
|:--|:--|
| `setup_workspace.m` | Populates the MATLAB base workspace with joint parameters (motor constants, gear ratios, link inertias) pulled from the project's single source of truth (see below). Run once at the start of every MATLAB session. |
| `load_urdf.m` | Wraps `smimport` to load the URDF from `software/ros2_ws/src/robot_description/` into a Simscape Multibody model. |
| `tune_all_joints.m` | Batch-runs PID Tuner on every joint controller and writes the resulting gains back into the parameter file. |
| `regenerate_codegen.m` | One-command rebuild of all generated C in `../codegen/output/`. Used in CI if MATLAB is available. |

## Joint parameter source of truth

To avoid divergence between the firmware constants, the URDF, and the Simulink workspace, joint parameters live in a single MATLAB data file (`scripts/joint_params.mat` — committed) that is generated from `docs/Constraints.md` and `docs/Calculations.md`. `setup_workspace.m` loads this file.

If you change a motor, gear ratio, or link mass:

1. Update `docs/Constraints.md` (and `Calculations.md` if torque math changes).
2. Update `scripts/joint_params.mat` to match.
3. Re-tune any affected controllers (PID Tuner).
4. Regenerate C code (`regenerate_codegen.m`).
5. Commit all of the above in the same PR.
