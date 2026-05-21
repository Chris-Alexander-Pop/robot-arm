$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Stm32Dir = Join-Path $RootDir "firmware/stm32_core"
$JointDir = Join-Path $RootDir "firmware/joint_node"

if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
    throw "PlatformIO CLI (pio) is not installed or not in PATH."
}

function Build-Stm32 {
    Write-Host "==> firmware/stm32_core (nucleo_f401re)"
    Set-Location $Stm32Dir
    pio run -e nucleo_f401re
}

function Build-JointDefault {
    Write-Host "==> firmware/joint_node (esp32dev)"
    Set-Location $JointDir
    pio run -e esp32dev
}

function Build-JointAll {
    Write-Host "==> firmware/joint_node (all node_* envs)"
    Set-Location $JointDir
    pio run -e node_j1 -e node_j2 -e node_j3 -e node_j4 -e node_j5 -e node_j6 -e node_gripper
}

$Target = if ($args.Count -gt 0) { $args[0] } else { "all" }

switch ($Target) {
    "all" {
        Build-Stm32
        Build-JointDefault
    }
    "stm32" { Build-Stm32 }
    "joint" { Build-JointDefault }
    "joint-all" { Build-JointAll }
    default {
        throw "Usage: build.ps1 [all|stm32|joint|joint-all]"
    }
}
