# Simulation Workspace

Python-first sandbox for validating kinematics and trajectory behavior before hardware deployment.

## Layout
- `robot_arm_sim/`: simulation package code.
- `scripts/`: executable entry scripts for local runs.
- `tests/`: unit tests for kinematics/planning components.
- `dh_solver.py`: initial DH prototype script.
- `requirements.txt`: Python dependencies.

## Onboarding
- Create the virtual environment with the repository setup scripts.
- Run tests on Linux/macOS with `simulation/scripts/test.sh`.
- Run tests on Windows with `simulation/scripts/test.ps1`.
