param(
    [Parameter(Position=0)]
    [ValidateSet("up", "down", "shell", "build", "launch")]
    [string]$Command
)

$ErrorActionPreference = "Stop"
$UseDockerComposePlugin = $true

Write-Host "=== Robot Arm Dev Helper (PowerShell) ==="

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Error "docker is not installed or not in PATH."
}

try {
    & docker compose version | Out-Null
} catch {
    if (Get-Command docker-compose -ErrorAction SilentlyContinue) {
        $UseDockerComposePlugin = $false
    } else {
        Write-Error "Neither 'docker compose' plugin nor 'docker-compose' is available."
    }
}

function Invoke-Compose {
    param(
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Args
    )

    if ($UseDockerComposePlugin) {
        & docker compose @Args
    } else {
        & docker-compose @Args
    }
}

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ProjectRoot

if (-not (Test-Path "software")) {
    Write-Error "Please run this script from the root of the robot-arm project directory."
}

switch ($Command) {
    "up" {
        Write-Host "Starting ROS 2 container in the background..."
        Set-Location "software"
        Invoke-Compose up -d
    }
    "down" {
        Write-Host "Stopping ROS 2 container..."
        Set-Location "software"
        Invoke-Compose down
    }
    "shell" {
        Write-Host "Opening interactive shell in ROS 2 container..."
        Set-Location "software"
        Invoke-Compose exec ros2 bash
    }
    "build" {
        Write-Host "Building ROS 2 workspace (colcon build)..."
        Set-Location "software"
        Invoke-Compose exec -w /ros2_ws ros2 bash -lc "source /opt/ros/humble/setup.bash && colcon build --log-base .log"
    }
    "launch" {
        Write-Host "Building workspace and launching RViz..."
        Set-Location "software"
        Invoke-Compose exec -w /ros2_ws ros2 bash -lc "source /opt/ros/humble/setup.bash && colcon build --log-base .log && source install/setup.bash && ros2 launch robot_description display.launch.py"
    }
    default {
        Write-Host "Usage: ./dev.ps1 [up|down|shell|build|launch]"
        exit 1
    }
}
