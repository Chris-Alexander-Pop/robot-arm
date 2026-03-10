# Robot Arm Design Validation & References

Building a 6-DOF robot arm is an awesome and challenging project! By keeping the heavy NEMA 17 motors closer to the base and using timing belts to transmit power to the elbow/wrist, you are following the exact methodology professional lightweight arms use to minimize their moving mass (moment of inertia).

## 1. Calculating Constraints (Torque & Lift Capacity)

The most critical calculation for a robot arm is the **Static Holding Torque** required at the **Shoulder (Joint 2)**. The worst-case scenario for the arm is when it is fully extended horizontally.

**The Math (Worst-Case Scenario):**
Torque ($\tau$) is Force $\times$ Distance. You need to calculate the torque exerted by the payload AND the weight of the arm itself:
$$ \tau_{shoulder} = (M_{payload} \times g \times L_{total}) + \sum (M_{link} \times g \times L_{center\_of\_mass}) $$
*   $M$ = Mass in kg
*   $g$ = Gravity ($9.81 m/s^2$)
*   $L$ = Distance from the shoulder joint in meters.

**Why Gearing is Critical:**
A typical NEMA 23 stepper motor has a holding torque of about $1$ to $3 \text{ Nm}$. 
If your arm is $0.5 \text{ meters}$ long and your payload is $1 \text{ kg}$ ($9.81 \text{ N}$), the payload alone requires $\approx 5 \text{ Nm}$ of torque at the shoulder—your NEMA 23 will stall immediately. 
*   **The Fix:** You need mechanical reduction. While timing belts are great, they usually only provide a $2:1$ or $3:1$ reduction before the pulleys get too large. For the Base and Shoulder joints, you usually need a **Planetary Gearbox** or **Harmonic Drive** with a $10:1$ to $50:1$ ratio attached to your NEMA 23 to multiply its torque.

## 2. Validation & Simulation

Before printing parts and buying expensive motors, you should simulate the design:

1.  **Iterative CAD Weights:** As you model the arm in Fusion360/SolidWorks/FreeCAD, assign materials (e.g., PETG) to your bodies. The CAD software will output exact link weights and center of mass locations. Plug those numbers into the torque equation above.
2.  **URDF and Physics Simulators:** You can write a URDF (Unified Robot Description Format) XML file defining your links (lengths, weights, inertia) and joints (rotation limits).
3.  **Simulation:** You can load this URDF into physics engines like **PyBullet**, **MuJoCo**, or **Gazebo (with ROS 2)**. You can command the simulated arm to lift a target payload and the simulator will output a graph showing exactly how much torque each joint needed.

## 3. Existing 6-DOF Open-Source Arms to Reference

Study these open-source arms, as they have solved many of the exact problems you are facing:

*   **Annin Robotics AR4 (The Gold Standard):** 
    *   *Why it's great:* It is the most robust, well-documented open-source 6-DOF arm out there. It uses NEMA steppers, planetary gearboxes, and aluminum/3D printed parts. 
    *   *What you can learn:* Look at their BOM (Bill of Materials) and manual. It will show you exactly what gear ratios they use for their NEMA 23s and 17s to achieve a 2kg payload.
*   **BCN3D Moveo:**
    *   *Why it's great:* It is a fully 3D-printed educational robotic arm. Crucially, **it uses timing belts and NEMA steppers precisely to transmit power across joints.**
    *   *What you can learn:* Check out their GitHub for their CAD files to see how they tensioned their GT2 belts and mounted their 608ZZ bearings.
*   **Niryo One / Niryo Ned:**
    *   *Why it's great:* A popular 6-DOF arm for education.
    *   *What you can learn:* They use stepper motors for the base/shoulder, and lightweight Dynamixel servos for the wrist/end-effector. This matches the idea of using servos for joints 5 & 6 to save weight!
*   **Thor Robot Arm (Hackaday):**
    *   *Why it's great:* Entirely 3D printed and open source. They use interesting planetary gear designs that you can 3D print yourself instead of buying expensive metal ones.
