$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$RenodeScript = Join-Path $RootDir "simulation/renode/nucleo_f401re.resc"
$LocalRenode = Join-Path $RootDir ".tooling/bin/renode.cmd"

if (Test-Path $LocalRenode) {
    $RenodeBin = $LocalRenode
} elseif (Get-Command renode -ErrorAction SilentlyContinue) {
    $RenodeBin = (Get-Command renode).Source
} else {
    throw "renode is not installed. Run .\setup.ps1 first."
}

${FirmwareElf} = (Join-Path $RootDir "firmware/stm32_core/.pio/build/genericSTM32F401CC_renode/firmware.elf") -replace '\\', '/'

Set-Location $RootDir
& $RenodeBin --disable-xwt --console -e "i @$RenodeScript; sysbus LoadELF @${FirmwareElf}; start"