# Renode STM32F401RE Scaffold

This directory holds the headless Renode entrypoints for firmware-level integration tests.

## Goal
- Run STM32F4-class firmware without a physical board.
- Keep Renode output in the terminal.
- Use UART output for firmware logs and assertions.

## Starting Point
- The current scaffold uses the generic STM32F4 CPU description as the board foundation.
- The Nucleo F401RE uses the STM32F4 family peripherals, so this is the correct baseline.
- If a more specific board description is available in your installed Renode package, swap it into the `.resc` script here.