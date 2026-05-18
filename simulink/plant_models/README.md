# Plant Models

> **Planned — not yet in repository** — no `.slx` plant models are checked in. See [`../../docs/implementation/simulink_workflow.md`](../../docs/implementation/simulink_workflow.md).

Simscape Multibody plant models of the arm joints. Each model captures the full electromechanical chain from driver step-pulse input to joint-angle output.

## Planned models

| File | Joint | Source dynamics |
|:--|:--|:--|
| `joint_J1_plant.slx` | J1 (Base) | NEMA 23 + CL57T (modeled as torque-speed curve) + 20:1 cycloidal (with backlash + viscous friction) + base-link inertia |
| `joint_J2_plant.slx` | J2 (Shoulder) | NEMA 23 + CL57T + 20:1 cycloidal + upper-arm inertia (gravity-loaded) |
| `arm_full_plant.slx` | All 6 | Simscape Multibody chain imported from URDF (`software/ros2_ws/src/robot_description/urdf/mock_arm.urdf.xacro`) |

## URDF import workflow

```matlab
% From the simulink/scripts/ directory:
smimport('../../software/ros2_ws/src/robot_description/urdf/mock_arm.urdf.xacro')
```

`smimport` produces a Simscape Multibody model with one Revolute Joint block per URDF joint. The imported model is then hand-augmented with:

- Motor torque-speed curves (from datasheets in `hardware/datasheets/`)
- Cycloidal backlash (modeled as a small dead-zone on torque transmission)
- Viscous + Coulomb friction (tuned to match real arm behavior — this is one of the URDF parameters that gets refined per the "simulation is the specification" philosophy)

## Validation

Plant models are validated against the constraints in [`../../docs/Constraints.md §1a`](../../docs/Constraints.md):

- Apply rated motor torque (2.0 Nm for NEMA 23) at the input.
- Verify output torque after 20:1 reduction matches the documented 40 Nm effective.
- Verify steady-state holding error under a 1 kg payload at full extension is within the ±0.5–1 mm repeatability target.

Mismatches between the plant model and the real arm get logged in [`../../docs/Journal.md`](../../docs/Journal.md) and drive URDF refinements.
