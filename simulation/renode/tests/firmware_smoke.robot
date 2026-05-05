*** Settings ***
Library    OperatingSystem

*** Variables ***
${ROOT}    ${CURDIR}/../../../

*** Test Cases ***
Firmware Boots In Renode
    Load Firmware
    Wait For Line On Uart    STM32 Robot Arm Controller Initializing...
    Wait For Line On Uart    STM32 Robot Arm Controller Ready.

Serial Command Updates Target Angle
    Load Firmware
    Wait For Line On Uart    STM32 Robot Arm Controller Initializing...
    Wait For Line On Uart    STM32 Robot Arm Controller Ready.
    Write To Uart    42.5\n
    Wait For Line On Uart    Received serial command: 42.5
    Wait For Line On Uart    New target angle: 42.50

*** Keywords ***
Load Firmware
    Log To Console    [Renode] Loading firmware and attaching to USART2
    Execute Command    set ROOT "${ROOT}"
    Execute Command    include @${ROOT}/simulation/renode/nucleo_f401re.resc
    ${ABS_ROOT}=    Normalize Path    ${ROOT}
    Execute Command    sysbus LoadELF @${ABS_ROOT}/firmware/stm32_core/.pio/build/nucleo_f401re_renode/firmware.elf
    Create Terminal Tester    sysbus.usart2
    Start Emulation