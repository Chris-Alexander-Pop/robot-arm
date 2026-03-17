$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent $PSScriptRoot
$SimDir = Join-Path $RootDir "simulation"
$VenvDir = Join-Path $SimDir ".venv"

if (Get-Command py -ErrorAction SilentlyContinue) {
    $PythonCmd = @("py", "-3")
} elseif (Get-Command python -ErrorAction SilentlyContinue) {
    $PythonCmd = @("python")
} else {
    throw "Python was not found in PATH."
}

Write-Host "Creating simulation virtual environment at $VenvDir"
& $PythonCmd[0] $PythonCmd[1..($PythonCmd.Length-1)] -m venv $VenvDir

$VenvPython = Join-Path $VenvDir "Scripts/python.exe"
& $VenvPython -m pip install --upgrade pip
& $VenvPython -m pip install -r (Join-Path $SimDir "requirements.txt")

Write-Host "Simulation virtual environment is ready."
Write-Host "Activate with: .\\simulation\\.venv\\Scripts\\Activate.ps1"
