$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $RootDir

Write-Host "=== Robot Arm Team Setup (PowerShell) ==="

if (Get-Command py -ErrorAction SilentlyContinue) {
    $UsePyLauncher = $true
} elseif (Get-Command python -ErrorAction SilentlyContinue) {
    $UsePyLauncher = $false
} else {
    throw "Python was not found in PATH."
}

function Invoke-Python {
    param(
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Args
    )

    if ($UsePyLauncher) {
        & py -3 @Args
    } else {
        & python @Args
    }
}

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Warning "docker is not installed or not in PATH. ROS container workflow will not work yet."
}

Write-Host "[1/4] Creating simulation virtual environment"
& (Join-Path $RootDir "scripts/create-venv.ps1")

Write-Host "[2/4] Creating local ROS tooling environment (.tooling)"
Invoke-Python -m venv (Join-Path $RootDir ".tooling")
$ToolingPython = Join-Path $RootDir ".tooling/Scripts/python.exe"
& $ToolingPython -m pip install --upgrade pip
& $ToolingPython -m pip install colcon-common-extensions vcstool rosdep

Write-Host "[3/4] Installing Renode"
Invoke-Python (Join-Path $RootDir "scripts/install-renode.py") --root $RootDir

Write-Host "[4/4] Verifying core commands"
$Colcon = Join-Path $RootDir ".tooling/Scripts/colcon.exe"
if (Test-Path $Colcon) {
    & $Colcon list --base-paths (Join-Path $RootDir "software/ros2_ws/src")
}

Write-Host "Setup complete."
Write-Host "Use './dev.ps1 up' to start ROS container and './dev.ps1 build' to build ROS packages."
