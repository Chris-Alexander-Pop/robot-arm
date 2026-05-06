param(
    [string]$Formats = "hs",
    [switch]$Clean,
    [switch]$Quiet,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Files
)

$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent $PSScriptRoot
$WvDir = Join-Path $RootDir "hardware/wireviz"
$OutDir = Join-Path $WvDir "out"
$VenvDir = Join-Path $WvDir ".venv"
$Wireviz = Join-Path $VenvDir "Scripts/wireviz.exe"

if (-not (Test-Path $Wireviz)) {
    throw "wireviz not found at $Wireviz. Run .\scripts\create-wireviz-venv.ps1 first."
}

if (-not (Get-Command dot -ErrorAction SilentlyContinue)) {
    throw "Graphviz 'dot' is not on PATH. Install graphviz and retry."
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
if ($Clean) {
    Write-Host "Cleaning $OutDir"
    Get-ChildItem -Path $OutDir -Force | Remove-Item -Recurse -Force
}

if ($Files -and $Files.Count -gt 0) {
    $Targets = @()
    foreach ($arg in $Files) {
        if (Test-Path $arg) {
            $Targets += (Resolve-Path $arg).Path
        } elseif (Test-Path (Join-Path $WvDir $arg)) {
            $Targets += (Join-Path $WvDir $arg)
        } elseif (Test-Path (Join-Path $WvDir "$arg.yml")) {
            $Targets += (Join-Path $WvDir "$arg.yml")
        } else {
            Write-Warning "Skip (not found): $arg"
        }
    }
} else {
    $Targets = Get-ChildItem -Path $WvDir -Filter "*.yml" | Select-Object -ExpandProperty FullName
}

if ($Targets.Count -eq 0) {
    throw "No YAML files to render."
}

$fail = 0
foreach ($src in $Targets) {
    $base = [System.IO.Path]::GetFileNameWithoutExtension($src)
    if ($Quiet) {
        & $Wireviz $src -o $OutDir -O $base -f $Formats *> $null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[ok]   $base"
        } else {
            Write-Host "[FAIL] $base"
            $fail++
        }
    } else {
        Write-Host "=== $base ==="
        & $Wireviz $src -o $OutDir -O $base -f $Formats
        if ($LASTEXITCODE -ne 0) { $fail++ }
    }
}

Write-Host ""
if ($fail -eq 0) {
    Write-Host "Rendered $($Targets.Count) file(s) into $OutDir"
} else {
    throw "Rendered with $fail failure(s); see output above."
}
