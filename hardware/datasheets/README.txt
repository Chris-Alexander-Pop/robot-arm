Vendor datasheet URLs for the robot arm BOM. PDFs are NOT stored in git (copyright).

Download locally into this folder if you want offline copies (files are gitignored):

  cd hardware/datasheets
  # example: curl -L -o TI_LM2596.pdf 'https://www.ti.com/lit/ds/symlink/lm2596.pdf'

If a download fails (TLS to st.com / ti.com can be flaky), open the link in a browser.

Power / analog / logic
  meanwell_LRS-350-spec.pdf
    https://www.meanwell.com/upload/pdf/LRS-350/LRS-350-SPEC.PDF
  TI_LM2596.pdf
    https://www.ti.com/lit/ds/symlink/lm2596.pdf
  TI_SN74HC245.pdf
    https://www.ti.com/lit/ds/symlink/sn74hc245.pdf
  TI_TCA9548A.pdf
    https://www.ti.com/lit/ds/symlink/tca9548a.pdf

Motors / drivers
  StepperOnline_CL57T_V40.pdf   (family doc; confirm against your V41 kit)
    https://www.omc-stepperonline.com/download/CL57T_V4.0.pdf
  StepperOnline_CL42T_manual.pdf
    https://www.omc-stepperonline.com/download/CL42T.pdf
  Trinamic_TMC2209_DigiKey_rev108.pdf   (Trinamic / Analog IC - verify YOUR breakout pins)
    https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/696/TMC2209_Rev.1.08.pdf

Sensors / homing
  Allegro_A314x.pdf   (A3144 family - legacy; consider a modern Hall part for new buys)
    https://www.allegromicro.com/~/media/Files/Datasheets/A3141-2-3-4-Datasheet.ashx
  AMS_AS5600.pdf   (AMS OSRAM AS5600 - Seeed-hosted copy)
    https://files.seeedstudio.com/wiki/Grove-12-bit-Magnetic-Rotary-Position-Sensor-AS5600/res/Magnetic%20Rotary%20Position%20Sensor%20AS5600%20Datasheet.pdf

MCU / board   (prefer official ST links in a browser)
  ST_RM0368_DM00096844_mirror.pdf   RM0368 reference manual for STM32F401xB/C/D/E
    Official: https://www.st.com/resource/en/reference_manual/dm00096844-stm32f401xb-c-and-stm32f401xd-e-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf
  ST_Nucleo64_UM1724_DM00105823_mirror.pdf   Nucleo-64 boards (MB1136), incl. NUCLEO-F401RE
    Official: https://www.st.com/resource/en/user_manual/dm00105823-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf

STM32F401RE chip datasheet:
  https://www.st.com/resource/en/datasheet/stm32f401re.pdf

MG996R gripper: no single vendor PDF; use seller diagrams + standard 3-wire hobby servo (GND, +5V, PWM).
