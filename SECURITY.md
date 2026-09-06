# Security Policy

This repository documents a **hobby / portfolio robotics project**, not a
production industrial robot. Do not use it for unattended operation or safety-critical
applications without independent risk assessment.

## Reporting

If you discover a security issue in project code or documentation, please open a
[GitHub Issue](https://github.com/Chris-Alexander-Pop/robot-arm/issues) with a
clear description. For sensitive reports, use GitHub private vulnerability reporting
if enabled on the repository.

## Residual GitHub object cache (Wi-Fi config)

`INDEX.md` item 1 was marked handled on 2026-08-04 after a history rewrite removed
the credential from every reachable ref. On 2026-09-01 that was found to be
incomplete: GitHub still serves the unreachable commit. Chris rotated the bench
WPA passphrase (the live network no longer accepts the string in that blob). This
agent did not re-run an association test. The passphrase is not repeated here.

GitHub Support cache-purge tickets were declined; none are recorded.

| Item | Value |
|---|---|
| Commit | `e8d15149ffaf19f5f34801697ebe9fd1e0ffc72e` |
| Blob | `3552d09047e00b8ad3dfc2c6a62c3ef535ac659d` |
| Path | `firmware/scripts/hardware_tests/rs485_esp32_bench/src/wifi_config.h` |
| Unauthenticated raw fetch | **HTTP 200** as of 2026-09-06 (`curl -sS -o /dev/null -w '%{http_code}'` against `raw.githubusercontent.com/.../<commit>/<path>` above; 743 bytes) |
| Forks retaining objects | 0 (`gh repo view Chris-Alexander-Pop/robot-arm --json forkCount`) |
| Rotation | **Done by Chris** (passphrase in the cached blob is residual, not a live credential to quote) |
| GitHub Support | Not filed (declined) |
| Offline backup | `~/Engineering/Toys/robot-arm-pre-purge-backup.git` (no remotes; holds the dangling commit) |

raw fetch: 200 as of 2026-09-06

The SSID is not in `HEAD` (env var `WIFI_SSID` in `bench_connect.sh`).

## Known limitations

- **Serial command interface**: The STM32 firmware accepts motion commands over UART
  without authentication or encryption. Any host connected to the serial port can
  command the arm. Treat the link as a trusted bench connection only.
- **No remote attack surface by design**: There is no cloud API or WiFi control stack
  in the current tree; scope is local ROS + serial.

## Supported versions

Only the current `main` branch is maintained during active development. Tagged
releases will be added when checkpoint demos are ready.
