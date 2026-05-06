# Renode STM32F401RE Scaffold

This directory holds the headless Renode entrypoints for firmware-level integration tests.

## Paths (portable clones)

- **`simulation/scripts/renode.sh`** / **`renode.ps1`** derive the repo root from the script location and load the `.resc` plus ELF under that root. Run them from any cwd after setup.
- **`tests/firmware_smoke.robot`** defaults `${ROOT}` to `${CURDIR}/../../../` (three levels up from `tests/`), which is the repo root regardless of where the tree lives. CI passes `--variable ROOT:${{ github.workspace }}` for the same effect on GitHub Actions.
- **`.tooling/`** (gitignored) holds a repo-local Renode install and a small wrapper script created by `scripts/install-renode.py`; those files contain absolute paths **only on your machine** and are regenerated when you run setup from a new clone path.

## Goal
- Run STM32F4-class firmware without a physical board.
- Keep Renode output in the terminal.
- Use UART output for firmware logs and assertions.

## Starting Point
- The current scaffold uses the generic STM32F4 CPU description as the board foundation.
- The Nucleo F401RE uses the STM32F4 family peripherals, so this is the correct baseline.
- If a more specific board description is available in your installed Renode package, swap it into the `.resc` script here.