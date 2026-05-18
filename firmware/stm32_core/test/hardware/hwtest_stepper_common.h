// Shared helpers for stepper hardware tests (laurb9/StepperDriver, MIT).
#pragma once

#ifdef ARDUINO

#include <BasicStepperDriver.h>

// Convert commanded speed (steps/s) to StepperDriver RPM for a given steps/rev.
inline float stepsPerSecToRpm(float speed_step_s, float steps_per_rev) {
  if (steps_per_rev <= 0.0F) {
    return 0.0F;
  }
  return speed_step_s * 60.0F / steps_per_rev;
}

// Run at constant speed for duration_ms (blocking). Negative speed = reverse.
inline void dwellAtSpeed(BasicStepperDriver& driver,
                         float speed_step_s,
                         float steps_per_rev,
                         unsigned long duration_ms) {
  driver.setSpeedProfile(BasicStepperDriver::CONSTANT_SPEED);
  driver.setRPM(stepsPerSecToRpm(speed_step_s >= 0.0F ? speed_step_s : -speed_step_s,
                                  steps_per_rev));
  const long steps = speed_step_s >= 0.0F ? 2000000L : -2000000L;
  driver.startMove(steps);
  const unsigned long start = millis();
  while (millis() - start < duration_ms) {
    driver.nextAction();
  }
  driver.stop();
  while (driver.getCurrentState() != BasicStepperDriver::STOPPED) {
    driver.nextAction();
  }
}

inline void rampToStop(BasicStepperDriver& driver, unsigned long duration_ms) {
  driver.startBrake();
  const unsigned long start = millis();
  while (millis() - start < duration_ms) {
    driver.nextAction();
    if (driver.getCurrentState() == BasicStepperDriver::STOPPED) {
      break;
    }
  }
  driver.stop();
  while (driver.getCurrentState() != BasicStepperDriver::STOPPED) {
    driver.nextAction();
  }
}

#endif  // ARDUINO
