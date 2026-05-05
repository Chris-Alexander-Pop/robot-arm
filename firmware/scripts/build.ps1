$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ProjectDir = Join-Path $RootDir "firmware/stm32_core"

if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    throw "PlatformIO CLI (pio) is not installed or not in PATH. Install the PlatformIO extension in VS Code or PlatformIO Core first."
}

Set-Location $ProjectDir
pio run -e nucleo_f401re