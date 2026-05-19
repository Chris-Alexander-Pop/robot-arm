$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$DevScript = Join-Path $RootDir "scripts/dev.ps1"

if (-not (Test-Path $DevScript)) {
    throw "scripts/dev.ps1 is missing."
}

& $DevScript up

& $DevScript moveit-test