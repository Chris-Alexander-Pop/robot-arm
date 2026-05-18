# Simulink Tests

Simulink Test harnesses for the plant models and controllers.

## Planned harnesses

| File | Tests |
|:--|:--|
| `plant_step_response.slx` | Apply a known torque step to each plant model and verify the steady-state velocity matches the expected motor torque-speed curve within ±5%. |
| `controller_tracking.slx` | Drive each closed-loop controller with a reference trajectory; assert tracking error stays within the spec in `docs/Constraints.md §3a`. |
| `codegen_equivalence.slx` | Run the original Simulink controller and the code-generated version against the same input sequence; assert outputs match bit-for-bit (Embedded Coder's standard "SIL" test). |

The `codegen_equivalence` test is the contract that lets us trust the generated C without re-validating it on hardware after every regeneration.

## Running

```matlab
sltest.testmanager.run('plant_step_response.mldatx')
```

Tests are run manually in MATLAB for now. CI integration is deferred — running headless MATLAB in CI requires either a self-hosted runner or MathWorks' GitHub Action (which requires a network license server).
