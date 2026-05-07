$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent $PSScriptRoot
$WvDir = Join-Path $RootDir "hardware/wireviz"
$VenvDir = Join-Path $WvDir ".venv"

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

Write-Host "Creating WireViz virtual environment at $VenvDir"
Invoke-Python -m venv $VenvDir

$Pip = Join-Path $VenvDir "Scripts/python.exe"
& $Pip -m pip install --upgrade pip
& $Pip -m pip install -r (Join-Path $WvDir "requirements.txt")

Write-Host "WireViz virtual environment is ready."
Write-Host "Activate with: .\hardware\wireviz\.venv\Scripts\Activate.ps1"
