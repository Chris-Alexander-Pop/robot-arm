<!--
Bill of Materials, multiple sections for each phase of development, update / append as seen fit
-->

| BOM Category | Component Specification & Rationale | Vendor | Unit Price (CAD) | Direct URL |
| :--- | :--- | :--- | :--- | :--- |
| **Motion Kit (Base/Shoulder)** | **1-CL57T-S20-V41 Closed-Loop Kit (Need 2)** - Includes 2.0Nm NEMA 23, CL57T driver (0-8.0A), and cables. Provides massive torque for J1/J2 cycloidal drives. | StepperOnline CA | $85.06 | [1-CL57T-S20-V41](https://www.stepperonline.ca/ts-series-2-0-nm-283-28oz-in-1-axis-closed-loop-stepper-cnc-kit-nema-23-motor-driver-1-cl57t-s20-v41.html) |
| **Motion Kit (Elbow)** | **1-CL42T-S08-V41 Closed-Loop Kit (Need 1)** - Includes 80Ncm NEMA 17, CL42T driver (0-3.0A), and cables. Heavy lifter for the mid-arm. | StepperOnline CA | $72.37 | [1-CL42T-S08-V41](https://www.stepperonline.ca/1-axis-closed-loop-stepper-cnc-kit-80ncm-113-29oz-in-nema-17-motor-driver-1-cl42t-s08-v41.html) |
| **Motion Kit (Forearm Twist)**| **1-CL42T-S04-V41 Closed-Loop Kit (Need 1)** - Includes 42Ncm NEMA 17, CL42T driver. Lighter weight to reduce the lever-arm effect on the elbow. | StepperOnline CA | $69.66 | [1-CL42T-S04-V41](https://www.omc-stepperonline.com/1-axis-closed-loop-stepper-cnc-kit-42ncm-59-48oz-in-nema-17-motor-driver-1-cl42t-s04-v41) |
| **Stepper Motor (Wrist)** | **14HS10-0404S Pancake Stepper (Need 2)** - Open-loop, 14Ncm, 0.4A. Ultra-lightweight (130g) and only 26mm long to eliminate dead weight at the J5/J6 end effector. | StepperOnline CA | $13.24 | [14HS10-0404S](https://www.stepperonline.ca/nema-14-bipolar-1-8deg-14ncm-20oz-in-0-4a-12v-35x35x26mm-4-wires-14hs10-0404s.html) |
| **Carrier Driver (Wrist)** | **TMC2209 Drivers (5-Pack)** - Handles the 0.4A NEMA 14s effortlessly. Silent operation and eliminates the need for bulky industrial drivers in the control box. | Amazon CA | ~$18.00 | [TMC2209 5-Pack](https://www.amazon.ca/s?k=TMC2209&i=industrial) |
| **Power Supply** | **Mean Well LRS-350-24** - 350W, 24V, 14.6A. Can handle the combined practical draw of 14.8A from all 6 motors (software protections can keep only 1 of the 2 biggest joints moving at once). | StepperOnline CA | $28.26 | [LRS-350-24](https://www.stepperonline.ca/lrs-350-24-mean-well-350w-24vdc-14-6a-115-230vac-enclosed-switching-power-supply-lrs-350-24.html) |
| **Structural Polymer** | **PETG Filament 1.75mm (1kg, Black)** - High glass transition temperature resists deformation from motor heat; superior layer adhesion over PLA/ABS. | RobotShop CA | $20.99 | [RobotShop Black PETG](https://ca.robotshop.com/products/3d-printing-canada-usa-black-standard-petg-filament-175mm-1kg) |
| **Cycloidal Output Bearings** | **6700/6800 Series Thin-Section Bearings** - Large-diameter (e.g., 6806 or 6808) to handle the structural bending forces of the arm at the joints. | Amazon / Sayal | ~$15.00 | Search: 6806 Bearing |
| **Cycloidal Input Bearings** | **608ZZ Bearings (10-Pack)** - Standard deep-groove bearings to act as the eccentric cam pushing the cycloidal disk. | Walmart CA | $9.69 | [608ZZ Bearings](https://www.walmart.ca/en/ip/Bearing-Tool-Accessories-10Pcs-608ZZ-Deep-Groove-Groove-Ball-Bearings-Ball-Bearingsfor-Printer-Rapid-Response/2K95UWZR387D) |
| **Cycloidal Ring Pins** | **Hardened Steel Dowel Pins** - 3mm to 5mm pins. Used as the outer ring gear teeth so the plastic disk doesn't shear off under load. | Amazon CA | ~$12.00 | Search: 4mm Dowel Pins |
| **Folded Kinematics** | **GT2 Belts & 36T Pulleys (8mm Bore)** - To mount the heavy J2/J3 motors parallel to the arm, fixing the center of gravity. | RobotShop CA | ~$13.00 | Search: GT2 Belt 6mm |
| **Structural Fasteners**| **Metric Alloy Steel Assortment** - Bulk kit of M3, M4, and M5 bolts/nuts to assemble the 3D-printed shells and drives. | VEVOR CA | $48.90 | [VEVOR Fastener Kit](https://www.vevor.ca/screws-c_13996/vevor-2250-pcs-bolts-nuts-assortment-kit-metric-imperial-alloy-steel-m3-m4-m5-p_010651610155) |
| **Threaded Anchors** | **M3x5mm Brass Heat-Set Inserts** - Melted into the PETG to provide permanent, high-torque metallic threads. | 3DPrintingCanada | $5.95 | [M3 Inserts](https://3dprintingcanada.com/products/m3x5-heat-set-brass-knurled-insert-nut-10-pack) |
| **Logic Power** | **LM2596 Buck Converter** - Steps the 24V rail down to 5V/3.3V to safely power the STM32 and wrist drivers. | Sayal / Amazon | ~$5.00 | Search: LM2596 Module |
| **Signal Buffer** | **74HC245 Octal Bus Transceiver** - Logic level shifter to bump the STM32's 3.3V pulses up to a clean 5V for the heavy industrial drivers. | Sayal / Amazon | ~$3.00 | Search: 74HC245 IC |
| **Homing Sensors** | **A3144 Hall Effect Sensors + Magnets** - Gives the STM32 a perfect absolute "Zero" coordinate on boot. | Sayal / Amazon | ~$8.00 | Search: A3144 Sensor |
| **End Effector** | **MG996R Micro Servo** - Metal-gear servo to actuate the gripper claw. | Sayal / Amazon | ~$10.00 | Search: MG996R Servo |
| **End Effector (Opt)** | **Force Sensitive Resistor (FSR)** - For implementing the force-feedback PID loop on the gripper fingertips. | Sayal / Amazon | ~$5.00 | Search: FSR Sensor |
| **Safety** | **E-Stop Button & IEC C14 Inlet** - Latching mushroom button and fused AC wall plug to safely cut power. | Sayal Electronics | ~$15.00 | In-Store |
| **Infrastructure (Wire)** | **18 AWG & 22 AWG Wire Spools** - 18 AWG silicone for motor power, 22 AWG shielded for encoder/logic signals. | Sayal Electronics | ~$20.00 | In-Store |
| **Infrastructure (Connectors)** | **GX16 Aviation Connectors** - Locking metal connectors for the control box outputs. | Sayal / Amazon | ~$10.00 | Search: GX16 Connectors |
| **Infrastructure (Terminals)** | **Wire Ferrules & Crimper** - Ensures perfect, non-fraying connections into the driver screw terminals. | Sayal / Amazon | ~$20.00 | Search: Ferrule Crimping Kit |
| **Cable Management** | **PET Sleeving & Drag Chain** - Bundles wires tightly and protects them from fatiguing during base sweeps. | Sayal / Amazon | ~$15.00 | Search: PET Sleeving |
| **Prototyping** | **Perfboard & Header Pins** - For making a permanent, vibration-proof hat for the STM32. | Sayal Electronics | ~$5.00 | In-Store |
| **Survival Gear** | **Synthetic PTFE Grease** - Packed into the cycloidal drives to prevent the PETG from melting via friction. | Hardware Store | ~$8.00 | In-Store |
| **Survival Gear** | **Blue Loctite** - Keeps the M3 bolts from vibrating out of the metal motor mounts. | Hardware Store | ~$6.00 | In-Store |

### Estimated Grand Total: ~$700.00 CAD - $800.00 CAD


### Prototyping & Dev BOM (Phase 1 Bench-Testing)

| BOM Category | Component Specification & Rationale | Vendor | Unit Price (CAD) | Direct URL |
| :--- | :--- | :--- | :--- | :--- |
| **Motion Kit (Base Size)** | **1-CL57T-S20-V41 Closed-Loop Kit (Need 1)** - 1x 2.0Nm NEMA 23 + Driver. Used to test the heavy cycloidal drive tolerances and max torque limits on the bench. | StepperOnline CA | $85.06 | [1-CL57T-S20-V41](https://www.stepperonline.ca/ts-series-2-0-nm-283-28oz-in-1-axis-closed-loop-stepper-cnc-kit-nema-23-motor-driver-1-cl57t-s20-v41.html) |
| **Motion Kit (Mid Size)** | **1-CL42T-S08-V41 Closed-Loop Kit (Need 1)** - 1x 80Ncm NEMA 17 + Driver. Used to dimension the mid-arm CAD and test the smaller elbow cycloidals. | StepperOnline CA | $72.37 | [1-CL42T-S08-V41](https://www.stepperonline.ca/1-axis-closed-loop-stepper-cnc-kit-80ncm-113-29oz-in-nema-17-motor-driver-1-cl42t-s08-v41.html) |
| **Stepper Motor (Wrist Size)**| **14HS10-0404S Pancake Stepper (Need 1)** - 1x 14Ncm NEMA 14. Used for modeling the end effector CAD and testing open-loop microstepping. | StepperOnline CA | $13.24 | [14HS10-0404S](https://www.stepperonline.ca/nema-14-bipolar-1-8deg-14ncm-20oz-in-0-4a-12v-35x35x26mm-4-wires-14hs10-0404s.html) |
| **Carrier Driver (Wrist)** | **TMC2209 Drivers (5-Pack)** - Need at least one to test the NEMA 14 logic from the STM32 via breadboard. Sold in packs anyway. | Amazon CA | ~$18.00 | [TMC2209 5-Pack](https://www.amazon.ca/s?k=TMC2209&i=industrial) |
| **Power Supply** | **Mean Well LRS-350-24** - Essential for bench testing. You can't drive the NEMA 23 to test cycloidal load without real 24V power. | StepperOnlineCA | $28.26 | [LRS-350-24](https://www.stepperonline.ca/lrs-350-24-mean-well-350w-24vdc-14-6a-115-230vac-enclosed-switching-power-supply-lrs-350-24.html) |
| **Cycloidal Output Bearings** | **6700/6800 Series Thin-Section Bearings** - Need a couple to prove out the structural output of your printed cycloidal prototypes. | Amazon / Sayal | ~$15.00 | Search: 6806 Bearing |
| **Cycloidal Input Bearings** | **608ZZ Bearings (10-Pack)** - Cheap pack to use as the eccentric cams for your bench tests. | Walmart CA | $9.69 | [608ZZ Bearings](https://www.walmart.ca/en/ip/Bearing-Tool-Accessories-10Pcs-608ZZ-Deep-Groove-Groove-Ball-Bearings-Ball-Bearingsfor-Printer-Rapid-Response/2K95UWZR387D) |
| **Cycloidal Ring Pins** | **Hardened Steel Dowel Pins** - Absolutely critical for the initial cycloidal bench testing to prevent shearing the 3d-printed ring gear. | Amazon CA | ~$12.00 | Search: 4mm Dowel Pins |
| **Folded Kinematics** | **GT2 Belts & 36T Pulleys (8mm Bore)** - Need at least one set to test the offset/folded design IF REQUIRED in CAD and physically. | RobotShop CA | ~$13.00 | Search: GT2 Belt 6mm |
| **Structural Fasteners**| **Metric Alloy Steel Assortment** - You can't assemble a prototype without bolts. May already have something similar. | VEVOR CA | $48.90 | [VEVOR Fastener Kit](https://www.vevor.ca/screws-c_13996/vevor-2250-pcs-bolts-nuts-assortment-kit-metric-imperial-alloy-steel-m3-m4-m5-p_010651610155) |
| **Threaded Anchors** | **M3x5mm Brass Heat-Set Inserts** - Need these immediately to test how well the PETG holds up to torque when bolted to the NEMA 23. | 3DPrintingCanada | $5.95 | [M3 Inserts](https://3dprintingcanada.com/products/m3x5-heat-set-brass-knurled-insert-nut-10-pack) |
| **Logic Power** | **LM2596 Buck Converter** - Mandatory for breadboard testing so you don't fry your STM32/Pi with 24V. | Sayal / Amazon | ~$5.00 | Search: LM2596 Module |
| **Signal Buffer** | **74HC245 Octal Bus Transceiver** - Need this on the breadboard to test reliable 5V logic shifting to the CL57T driver. | Sayal / Amazon | ~$3.00 | Search: 74HC245 IC |
| **Homing Sensors** | **A3144 Hall Effect Sensors + Magnets** - Grab a few to prototype the homing interrupt logic on the STM32. | Sayal / Amazon | ~$8.00 | Search: A3144 Sensor |
| **Safety** | **E-Stop Button & IEC C14 Inlet** - Don't plug bare wires into the wall for a bench test. Wire these up first. | Sayal Electronics | ~$15.00 | In-Store |
| **Survival Gear** | **Synthetic PTFE Grease** - Must apply this during bench tests, or the cycloidal prototype will melt itself. | Hardware Store | ~$8.00 | In-Store |

### Estimated Dev Phase Total: ~$350.00 CAD - $400.00 CAD

*(Note: Stripped out the extra motors, extra kits, servo/FSR, wire sleeving, aviation connectors, and permanent perfboards. This is strictly the hardware needed to prove the electronics on a breadboard and successfully CAD/print a working, load-tested cycloidal joint).*