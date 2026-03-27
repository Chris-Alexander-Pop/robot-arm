param(
    [Parameter(Position=0)]
    [ValidateSet("up", "down", "shell", "build", "launch", "moveit-sim", "moveit-real", "moveit-test")]
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
        Invoke-Compose exec -w /ros2_ws ros2 bash -lc "source /opt/ros/humble/setup.bash && colcon build"
    }
    "launch" {
        Write-Host "Building workspace and launching RViz..."
        Set-Location "software"
        Invoke-Compose exec -w /ros2_ws ros2 bash -lc "source /opt/ros/humble/setup.bash && colcon build && source install/setup.bash && ros2 launch robot_description display.launch.py"
    }
    "moveit-sim" {
        Write-Host "Building workspace and launching MoveIt in simulation mode..."
        Set-Location "software"
        Invoke-Compose exec -w /ros2_ws ros2 bash -lc "source /opt/ros/humble/setup.bash && colcon build --packages-select robot_description robot_arm_moveit && source install/setup.bash && ros2 launch robot_arm_moveit sim.launch.py"
    }
    "moveit-real" {
        Write-Host "Building workspace and launching MoveIt in real-hardware mode..."
        Set-Location "software"
        Invoke-Compose exec -w /ros2_ws ros2 bash -lc "source /opt/ros/humble/setup.bash && colcon build --packages-select robot_description robot_arm_moveit && source install/setup.bash && ros2 launch robot_arm_moveit real.launch.py"
    }
    "moveit-test" {
        Write-Host "Running MoveIt smoke tests..."
        Set-Location "software"
        Invoke-Compose exec -w /ros2_ws ros2 bash -lc "source /opt/ros/humble/setup.bash && colcon build --packages-select robot_description robot_arm_moveit && colcon test --packages-select robot_arm_moveit --event-handlers console_direct+ && colcon test-result --verbose"
    }
    default {
        Write-Host "Usage: ./dev.ps1 [up|down|shell|build|launch|moveit-sim|moveit-real|moveit-test]"
        exit 1
    }
}
