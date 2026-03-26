$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ProjectDir = Join-Path $RootDir "firmware/stm32_core"

if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    throw "PlatformIO CLI (pio) is not installed or not in PATH."
}

Set-Location $ProjectDir
pio test -e native