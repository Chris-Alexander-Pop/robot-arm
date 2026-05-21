$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$VenvPython = Join-Path $RootDir "simulation/.venv/Scripts/python.exe"

if (-not (Test-Path $VenvPython)) {
    throw "simulation virtual environment not found. Run .\scripts\setup.ps1 first."
}

Set-Location (Join-Path $RootDir "simulation")
& $VenvPython -m pytest