*** Settings ***
Library    OperatingSystem

*** Variables ***
${ROOT}    ${CURDIR}/../../../

*** Test Cases ***
Firmware Boots In Renode
    Execute Command    set ROOT "${ROOT}"
    Execute Command    include @${ROOT}/simulation/renode/nucleo_f401re.resc
    ${ABS_ROOT}=    Normalize Path    ${ROOT}
    Execute Command    sysbus LoadELF @${ABS_ROOT}/firmware/stm32_core/.pio/build/genericSTM32F401CC/firmware.elf
    Create Terminal Tester    sysbus.usart2
    Start Emulation
    Wait For Line On Uart    STM32 Robot Arm Controller Initializing...