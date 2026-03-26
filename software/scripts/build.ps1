$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$DevScript = Join-Path $RootDir "dev.ps1"

if (-not (Test-Path $DevScript)) {
    throw "dev.ps1 is missing. Run from the repository root workflow."
}

& $DevScript build