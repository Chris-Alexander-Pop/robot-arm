# Embedded Codegen

Embedded Coder configuration and generated C source for the joint controllers. The generated code is checked in so the firmware (`firmware/stm32_core/`) builds without requiring a MATLAB license.

## Layout

```
codegen/
├── README.md                     # ← this file
├── joint_pid_codegen.slx         # Wrapper model with codegen configuration
├── ert_main.tmpl                 # Custom main template (we provide our own integration)
└── output/                       # Generated C (COMMITTED)
    ├── joint_J1_pid/
    │   ├── joint_J1_pid.c
    │   ├── joint_J1_pid.h
    │   └── joint_J1_pid_types.h
    └── joint_J2_pid/
        └── ...
```

## Code Generation Settings

The wrapper model `joint_pid_codegen.slx` references the controllers from `../controllers/` and applies the following Embedded Coder configuration:

| Setting | Value | Why |
|:--|:--|:--|
| System target file | `ert.tlc` (Embedded Real-Time) | Generates production-quality C, not the larger GRT runtime |
| Hardware Implementation | ARM Cortex-M | Matches STM32 target |
| Floating point support | Single precision (`float32`) | Matches STM32F4 FPU |
| Generate reusable code | On | Lets us call the same function for multiple joints with different parameter structs |
| Pack Boolean data into bitfields | Off | Keeps the generated code easy to inspect in code review |
| File packaging format | Modular | One .c/.h per subsystem, not a single monolithic file |
| Code interface | Reusable function with parameter argument | Generated signature matches the hand-written firmware adapter |

## Integration with PlatformIO firmware

Generated source under `output/` is symlinked (or copied — TBD) into:

```
firmware/stm32_core/lib/control/generated/
```

A hand-written adapter at `firmware/stm32_core/lib/control/src/joint_controller_generated.cpp` instantiates one parameter struct per joint, calls the generated step function at 1 kHz from the existing `JointController::Step` hook, and routes the output back into the existing `StepperDriver` pulse-rate setpoint. The adapter is the **only** hand-written firmware code that touches the generated functions — everything else in the firmware stays decoupled.

This boundary is enforced so a future controller redesign (e.g., switching to LQR) does not require firmware changes outside the adapter.

## Regeneration

```matlab
% From simulink/codegen/:
load_system('joint_pid_codegen.slx');
rtwbuild('joint_pid_codegen');
```

After regeneration, commit `output/` along with the `.slx` change.

**Never edit files under `output/` by hand.** They are overwritten on every regeneration. If you need to change controller behavior, edit the model in `../controllers/`.
