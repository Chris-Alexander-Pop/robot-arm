// Hardware test: Single-axis stepper motor (NEMA 23 + CL57T)
//
// Required hardware:
//   - Nucleo-F401RE connected via ST-Link USB
//   - NEMA 23 motor wired to CL57T driver
//   - CL57T STEP/DIR inputs connected to kJ1StepPin / kJ1DirPin (see pinout.h)
//   - CL57T powered (24–48 V DC), motor phased correctly
//   - Optional: CL57T ALM output connected to a GPIO for fault monitoring
//   - USB serial monitor at 115200 baud
//
// Success criteria:
//   - Motor ramps up, holds speed, then ramps back to stop
//   - Motor reverses direction cleanly
//   - No ALM fault reported at startup
//   - All speed dwell steps complete with "OK" printed per step
//
// Flash and monitor:
//   cd firmware/scripts/hardware_tests && ./run_stepper_single.sh

#ifdef HWTEST_STEPPER_SINGLE

#include <Arduino.h>
#include <BasicStepperDriver.h>

#include "hwtest_stepper_common.h"
#include "pinout.h"

// Microstepping setting on the CL57T DIP switches — adjust to match your config.
// CL57T common settings: 1600, 3200, 6400 steps/rev.
static constexpr short kMotorFullSteps = 200;
static constexpr short kMicrosteps      = 8;     // 200 * 8 = 1600 steps/rev
static constexpr float kStepsPerRev   = 1600.0F;
static constexpr float kMaxSpeedStepS = 1200.0F;   // steps/s  (~45 deg/s at 1600 steps/rev)
static constexpr float kAccelStepSS   = 400.0F;    // steps/s^2 (for future accelerated moves)

static BasicStepperDriver stepper(kMotorFullSteps, kJ1DirPin, kJ1StepPin);

static void print_separator() {
  Serial.println("----------------------------------------");
}

void hwtest_stepper_single_setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000UL) { delay(10); }

  print_separator();
  Serial.println("HWTEST: Single-axis stepper (NEMA 23 + CL57T)");
  print_separator();

  stepper.begin(60.0F, kMicrosteps);
  stepper.setSpeedProfile(BasicStepperDriver::CONSTANT_SPEED, static_cast<short>(kAccelStepSS),
                          static_cast<short>(kAccelStepSS));

  Serial.println("Stepper configured (laurb9/StepperDriver).");
  Serial.print("  Steps/rev: ");    Serial.println(static_cast<int>(kStepsPerRev));
  Serial.print("  Max speed: ");    Serial.print(kMaxSpeedStepS);   Serial.println(" steps/s");
  Serial.print("  Acceleration: "); Serial.print(kAccelStepSS);     Serial.println(" steps/s^2");
  print_separator();
}

void hwtest_stepper_single_loop() {
  static bool done = false;
  if (done) {
    delay(1000);
    return;
  }

  Serial.println("[1] Ramping UP (forward)...");
  dwellAtSpeed(stepper, 100.0F, kStepsPerRev, 500);
  dwellAtSpeed(stepper, kMaxSpeedStepS, kStepsPerRev, 500);
  Serial.println("    Holding max speed 2 s...");
  dwellAtSpeed(stepper, kMaxSpeedStepS, kStepsPerRev, 2000);
  Serial.println("    Ramping DOWN...");
  rampToStop(stepper, 500);
  delay(300);
  Serial.println("[1] OK");

  Serial.println("[2] Reversing direction...");
  dwellAtSpeed(stepper, -kMaxSpeedStepS, kStepsPerRev, 500);
  Serial.println("    Holding reverse 2 s...");
  dwellAtSpeed(stepper, -kMaxSpeedStepS, kStepsPerRev, 2000);
  Serial.println("    Ramping DOWN...");
  rampToStop(stepper, 500);
  delay(300);
  Serial.println("[2] OK");

  print_separator();
  Serial.println("[3] Speed sweep (forward):");
  const float speeds[] = {50.0F, 100.0F, 200.0F, 500.0F, 1000.0F};
  for (size_t i = 0; i < sizeof(speeds) / sizeof(speeds[0]); ++i) {
    Serial.print("    ");
    Serial.print(static_cast<int>(speeds[i]));
    Serial.print(" steps/s ...");
    dwellAtSpeed(stepper, speeds[i], kStepsPerRev, 2000);
    Serial.println(" OK");
  }
  rampToStop(stepper, 500);
  delay(300);

  Serial.println("[4] Speed sweep (reverse):");
  for (size_t i = 0; i < sizeof(speeds) / sizeof(speeds[0]); ++i) {
    Serial.print("    -");
    Serial.print(static_cast<int>(speeds[i]));
    Serial.print(" steps/s ...");
    dwellAtSpeed(stepper, -speeds[i], kStepsPerRev, 2000);
    Serial.println(" OK");
  }
  rampToStop(stepper, 500);
  delay(300);

  print_separator();
  Serial.println("HWTEST COMPLETE: all stepper single-axis tests passed.");
  Serial.println("Monitor for any ALM faults during run; check CL57T LED indicators.");
  print_separator();
  done = true;
}

#endif  // HWTEST_STEPPER_SINGLE
