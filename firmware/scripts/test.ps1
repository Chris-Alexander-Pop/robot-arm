$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ProjectDir = Join-Path $RootDir "firmware/stm32_core"

if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    throw "PlatformIO CLI (pio) is not installed or not in PATH."
}

Set-Location $ProjectDir
pio run -e native

$NativeProgram = Join-Path $ProjectDir ".pio/build/native/program"
$NativeProgramExe = Join-Path $ProjectDir ".pio/build/native/program.exe"

if (Test-Path $NativeProgram) {
    & $NativeProgram
} elseif (Test-Path $NativeProgramExe) {
    & $NativeProgramExe
} else {
    throw "Native test program was not built."
}