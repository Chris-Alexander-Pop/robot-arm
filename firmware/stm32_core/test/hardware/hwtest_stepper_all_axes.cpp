// Hardware test: All 6 axes sequential jog
//
// Required hardware:
//   - Nucleo-F401RE connected via ST-Link USB
//   - All 6 stepper drivers (CL57T / CL42T) powered and wired
//   - STEP/DIR pairs per pinout.h (J1-J6)
//   - USB serial monitor at 115200 baud
//
// Success criteria:
//   - Each joint moves briefly in both directions and reports "OK"
//   - No joint skips (missing STEP pulses) — motor should move smoothly
//   - Confirm each motor responds to the correct STEP/DIR pair
//   - Use as a final pinout.h wiring verification before software bring-up
//
// Flash and monitor:
//   cd firmware/scripts/hardware_tests && ./run_stepper_all.sh

#ifdef HWTEST_STEPPER_ALL_AXES

#include <Arduino.h>
#include <AccelStepper.h>

#include "core/types.h"
#include "pinout.h"

static constexpr float kJogSpeedStepS  = 400.0F;   // steps/s for jog moves
static constexpr unsigned long kJogMs  = 1500UL;   // dwell time at jog speed

static AccelStepper steppers[robot_arm::kJointCount] = {
  AccelStepper(AccelStepper::DRIVER, kJ1StepPin, kJ1DirPin),
  AccelStepper(AccelStepper::DRIVER, kJ2StepPin, kJ2DirPin),
  AccelStepper(AccelStepper::DRIVER, kJ3StepPin, kJ3DirPin),
  AccelStepper(AccelStepper::DRIVER, kJ4StepPin, kJ4DirPin),
  AccelStepper(AccelStepper::DRIVER, kJ5StepPin, kJ5DirPin),
  AccelStepper(AccelStepper::DRIVER, kJ6StepPin, kJ6DirPin),
};

static void jog_joint(int joint_idx, float speed_step_s, unsigned long ms) {
  steppers[joint_idx].setSpeed(speed_step_s);
  const unsigned long start = millis();
  while (millis() - start < ms) {
    steppers[joint_idx].runSpeed();
  }
  steppers[joint_idx].setSpeed(0.0F);
}

void hwtest_stepper_all_axes_setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000UL) { delay(10); }

  Serial.println("========================================");
  Serial.println("HWTEST: All-axes sequential jog");
  Serial.println("========================================");

  for (int j = 0; j < robot_arm::kJointCount; ++j) {
    steppers[j].setMaxSpeed(kJogSpeedStepS * 1.5F);
    steppers[j].setSpeed(0.0F);
  }

  Serial.println("All steppers configured. Starting jog sequence...");
  Serial.println();
}

void hwtest_stepper_all_axes_loop() {
  static bool done = false;
  if (done) {
    delay(1000);
    return;
  }

  for (int j = 0; j < robot_arm::kJointCount; ++j) {
    Serial.print("Joint J");
    Serial.print(j + 1);
    Serial.print(": forward jog...");

    jog_joint(j, kJogSpeedStepS, kJogMs);
    delay(200);

    Serial.print(" reverse jog...");
    jog_joint(j, -kJogSpeedStepS, kJogMs);
    delay(200);

    Serial.println(" OK");
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("HWTEST COMPLETE: all-axes jog finished.");
  Serial.println("Verify each joint moved in the expected direction.");
  Serial.println("If a joint did not move, check:");
  Serial.println("  - STEP/DIR wiring against pinout.h");
  Serial.println("  - Driver enable/power status");
  Serial.println("  - Driver fault (ALM) LED");
  Serial.println("========================================");
  done = true;
}

#endif  // HWTEST_STEPPER_ALL_AXES
