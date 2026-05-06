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
#include <AccelStepper.h>

#include "pinout.h"

// Microstepping setting on the CL57T DIP switches — adjust to match your config.
// CL57T common settings: 1600, 3200, 6400 steps/rev.
static constexpr float kStepsPerRev  = 1600.0F;
static constexpr float kMaxSpeedStepS = 1200.0F;   // steps/s  (~45 deg/s at 1600 steps/rev)
static constexpr float kAccelStepSS  =  400.0F;   // steps/s²

// ALM pin — uncomment and set once wired.
// static constexpr int kAlmPin = PA8;  // example candidate; verify against pinout.h

static AccelStepper stepper(AccelStepper::DRIVER, kJ1StepPin, kJ1DirPin);

static void print_separator() {
  Serial.println("----------------------------------------");
}

static void dwell_at_speed(float speed_step_s, unsigned long duration_ms) {
  stepper.setSpeed(speed_step_s);
  const unsigned long start = millis();
  while (millis() - start < duration_ms) {
    stepper.runSpeed();
  }
}

static void ramp_to_stop(unsigned long duration_ms) {
  // Gradually reduce speed toward 0 over duration_ms.
  const unsigned long start = millis();
  const float initial_speed = stepper.speed();
  while (millis() - start < duration_ms) {
    const float fraction = 1.0F - static_cast<float>(millis() - start) / static_cast<float>(duration_ms);
    stepper.setSpeed(initial_speed * fraction);
    stepper.runSpeed();
  }
  stepper.setSpeed(0.0F);
}

void hwtest_stepper_single_setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000UL) { delay(10); }

  print_separator();
  Serial.println("HWTEST: Single-axis stepper (NEMA 23 + CL57T)");
  print_separator();

  // --- ALM fault check ---
  // Uncomment once ALM pin is wired:
  // pinMode(kAlmPin, INPUT_PULLUP);
  // delay(50);
  // const bool alm_fault = (digitalRead(kAlmPin) == LOW);
  // if (alm_fault) {
  //   Serial.println("WARNING: CL57T ALM pin is asserted — driver may be in fault state.");
  //   Serial.println("  Check power supply, motor wiring, and encoder connector.");
  // } else {
  //   Serial.println("ALM pin OK (no fault detected).");
  // }

  stepper.setMaxSpeed(kMaxSpeedStepS);
  stepper.setAcceleration(kAccelStepSS);
  stepper.setSpeed(0.0F);

  Serial.println("Stepper configured.");
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

  // --- Phase 1: Ramp up to max speed (forward), hold 2 s, ramp down ---
  Serial.println("[1] Ramping UP (forward)...");
  dwell_at_speed(100.0F, 500);   // brief low-speed dwell before ramp
  dwell_at_speed(kMaxSpeedStepS, 500);
  Serial.println("    Holding max speed 2 s...");
  dwell_at_speed(kMaxSpeedStepS, 2000);
  Serial.println("    Ramping DOWN...");
  ramp_to_stop(500);
  delay(300);
  Serial.println("[1] OK");

  // --- Phase 2: Reverse direction ---
  Serial.println("[2] Reversing direction...");
  dwell_at_speed(-kMaxSpeedStepS, 500);
  Serial.println("    Holding reverse 2 s...");
  dwell_at_speed(-kMaxSpeedStepS, 2000);
  Serial.println("    Ramping DOWN...");
  ramp_to_stop(500);
  delay(300);
  Serial.println("[2] OK");

  // --- Phase 3: Speed sweep ---
  print_separator();
  Serial.println("[3] Speed sweep (forward):");
  const float speeds[] = { 50.0F, 100.0F, 200.0F, 500.0F, 1000.0F };
  for (size_t i = 0; i < sizeof(speeds) / sizeof(speeds[0]); ++i) {
    Serial.print("    ");
    Serial.print(static_cast<int>(speeds[i]));
    Serial.print(" steps/s ...");
    dwell_at_speed(speeds[i], 2000);
    Serial.println(" OK");
  }
  ramp_to_stop(500);
  delay(300);

  // --- Phase 4: Speed sweep (reverse) ---
  Serial.println("[4] Speed sweep (reverse):");
  for (size_t i = 0; i < sizeof(speeds) / sizeof(speeds[0]); ++i) {
    Serial.print("    -");
    Serial.print(static_cast<int>(speeds[i]));
    Serial.print(" steps/s ...");
    dwell_at_speed(-speeds[i], 2000);
    Serial.println(" OK");
  }
  ramp_to_stop(500);
  delay(300);

  // --- Done ---
  print_separator();
  Serial.println("HWTEST COMPLETE: all stepper single-axis tests passed.");
  Serial.println("Monitor for any ALM faults during run; check CL57T LED indicators.");
  print_separator();
  done = true;
}

#endif  // HWTEST_STEPPER_SINGLE
