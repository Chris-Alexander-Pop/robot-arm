# Firmware

Domain-level firmware area for low-level embedded control.

## Important location
- `stm32_core/`: STM32 PlatformIO project (control loop, drivers, protocol).

## Onboarding
- Open `firmware/stm32_core/` as the PlatformIO project root.
- Build from Linux/macOS with `firmware/scripts/build.sh`.
- Build from Windows with `firmware/scripts/build.ps1`.
- Use the PlatformIO VS Code extension so `Arduino.h` and other board headers resolve correctly.
- If IntelliSense still reports missing headers, run `pio run -t compiledb` once inside `firmware/stm32_core/` and reload VS Code.

## Testing Strategy
- Use C++ unit tests for firmware logic that can run on the host or in PlatformIO's native test environment.
- Run them with `firmware/scripts/test.sh` or `firmware/scripts/test.ps1`.
- Use Renode for hardware-free STM32 integration tests when you need to exercise the firmware against an emulated MCU.
- Launch it from `simulation/scripts/renode.sh` or `simulation/scripts/renode.ps1` to keep output in the terminal.
- Use Python only for test orchestration, log parsing, or higher-level simulation harnesses.
- Treat `firmware/stm32_core/src/main.cpp` as application code, and keep logic you want to unit test in `lib/`.

## Scope
- Real-time motor control and encoder feedback.
- MCU-side host communication protocol.
