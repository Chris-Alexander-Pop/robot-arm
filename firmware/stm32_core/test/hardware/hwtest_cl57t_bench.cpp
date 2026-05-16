// Hardware test: CL57T bench (PA0=STEP, PA1=DIR). Press B1 to start.

#ifdef HWTEST_CL57T_BENCH

#include <Arduino.h>

#include "pinout.h"

static constexpr uint8_t kStepPin = kBenchStepPin;
static constexpr uint8_t kDirPin  = kBenchDirPin;
// static constexpr uint8_t kStatusLedPin = PA5;  // used by creep + pin check only

static constexpr float kStepsPerRev = 1600.0F;
static constexpr unsigned long kDwellMs = 2000UL;
static constexpr unsigned int kStepPulseUs = 50U;

// Bench speeds (steps/s). 9–12 are 2x–5x of speed 8 (1500/s).
static constexpr float kSpeed8StepS = 1500.0F;
static constexpr float kSpeeds[] = {
    50.0F,   100.0F,  200.0F,  350.0F,  500.0F,  750.0F,  1000.0F, kSpeed8StepS,
    kSpeed8StepS * 2.0F,  // 9:  3000/s
    kSpeed8StepS * 3.0F,  // 10: 4500/s
    kSpeed8StepS * 4.0F,  // 11: 6000/s
    kSpeed8StepS * 5.0F,  // 12: 7500/s
};
static constexpr const char* kSpeedLabels[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12",
};
static constexpr size_t kSpeedCount = sizeof(kSpeeds) / sizeof(kSpeeds[0]);

enum class BenchState { WaitForButton, Running, Done };

static BenchState state = BenchState::WaitForButton;

static void print_banner() {
  Serial.println();
  Serial.println("========================================");
  Serial.println(" HWTEST: CL57T bench (PA0=STEP PA1=DIR)");
  Serial.println("========================================");
  Serial.print("  Steps/rev (DIP): ");
  Serial.println(static_cast<int>(kStepsPerRev));
  Serial.print("  Speed steps: ");
  Serial.print(kSpeeds[0], 0);
  Serial.print(" .. ");
  Serial.print(kSpeeds[kSpeedCount - 1], 0);
  Serial.println(" steps/s");
  Serial.println("  >>> Press B1 (blue USER button) to start <<<");
  Serial.println("========================================");
}

static bool user_button_pressed() {
  return digitalRead(kUserButtonPin) == LOW;
}

static bool wait_for_button_press() {
  if (!user_button_pressed()) {
    return false;
  }
  delay(30);
  if (!user_button_pressed()) {
    return false;
  }
  while (user_button_pressed()) {
    delay(10);
  }
  return true;
}

#if 0  // --- bring-up diagnostics (disabled) ---
static void pin_check_square_wave() {
  Serial.println();
  Serial.println("[pin check] PA0 toggling 1 Hz for 10 s (DMM ~1.6 V on A0)");
  const unsigned long t0 = millis();
  bool level = false;
  while (millis() - t0 < 10000UL) {
    level = !level;
    digitalWrite(kStepPin, level ? HIGH : LOW);
    digitalWrite(kStatusLedPin, level ? HIGH : LOW);
    delay(500);
  }
  digitalWrite(kStepPin, LOW);
  digitalWrite(kStatusLedPin, LOW);
  Serial.println("[pin check] PA1 HIGH 2 s (~3.3 V on A1) ...");
  digitalWrite(kDirPin, HIGH);
  delay(2000);
  Serial.println("[pin check] PA1 LOW 2 s (~0 V on A1) ...");
  digitalWrite(kDirPin, LOW);
  delay(2000);
  Serial.println("[pin check] OK");
}
#endif

static void emit_step_pulse() {
  digitalWrite(kStepPin, HIGH);
  delayMicroseconds(kStepPulseUs);
  digitalWrite(kStepPin, LOW);
}

static unsigned long run_visible_creep(float steps_per_sec,
                                       unsigned long duration_ms,
                                       bool forward,
                                       unsigned int pulse_ms) {
  if (steps_per_sec <= 0.0F) {
    return 0;
  }

  digitalWrite(kDirPin, forward ? HIGH : LOW);

  const unsigned long interval_ms =
      static_cast<unsigned long>(1000.0F / steps_per_sec);
  const unsigned long t_end = millis() + duration_ms;
  unsigned long count = 0;

  while (millis() < t_end) {
    const unsigned long step_start = millis();
    digitalWrite(kStepPin, HIGH);
    delay(pulse_ms);
    digitalWrite(kStepPin, LOW);
    ++count;

    while (millis() - step_start < interval_ms) {
    }
  }
  return count;
}

static unsigned long run_at_speed(float steps_per_sec,
                                  unsigned long duration_ms,
                                  bool forward) {
  if (steps_per_sec <= 0.0F) {
    return 0;
  }

  digitalWrite(kDirPin, forward ? HIGH : LOW);

  const unsigned long interval_us =
      static_cast<unsigned long>(1000000.0F / steps_per_sec);
  unsigned long last_pulse_us = micros();
  const unsigned long t_end = millis() + duration_ms;
  unsigned long count = 0;

  while (millis() < t_end) {
    const unsigned long now = micros();
    if (now - last_pulse_us >= interval_us) {
      emit_step_pulse();
      last_pulse_us = now;
      ++count;
    }
  }
  return count;
}

static void coast_to_stop() {
  digitalWrite(kStepPin, LOW);
  delay(200);
}

static void run_phase(const char* label, float speed_step_s, bool forward) {
  Serial.print("  ");
  Serial.print(label);
  Serial.print(" @ ");
  Serial.print(speed_step_s, 0);
  Serial.print(" steps/s ... ");
  const unsigned long n = run_at_speed(speed_step_s, kDwellMs, forward);
  coast_to_stop();
  Serial.print("OK (");
  Serial.print(n);
  Serial.println(" pulses)");
  delay(300);
}

static void run_speed_sweep(bool forward) {
  for (size_t i = 0; i < kSpeedCount; ++i) {
    char label[16];
    snprintf(label, sizeof(label), "speed %s", kSpeedLabels[i]);
    run_phase(label, kSpeeds[i], forward);
  }
}

static void run_motion_sequence() {
  Serial.println();
  Serial.println("GO — creep, then speed 1..12 forward and reverse.");
  Serial.println("----------------------------------------");

  Serial.println("[0] Creep (1 step/s, 5 s)");
  const unsigned long creep_n = run_visible_creep(1.0F, 5000UL, true, 200U);
  coast_to_stop();
  Serial.print("    creep pulses: ");
  Serial.println(creep_n);
  delay(400);

  Serial.println("[1] Forward");
  run_speed_sweep(true);

  Serial.println("[2] Reverse");
  run_speed_sweep(false);

  digitalWrite(kStepPin, LOW);
  digitalWrite(kDirPin, LOW);

  Serial.println("----------------------------------------");
  Serial.println("HWTEST COMPLETE");
  Serial.println("----------------------------------------");
}

void hwtest_cl57t_bench_setup() {
  pinMode(kStepPin, OUTPUT);
  pinMode(kDirPin, OUTPUT);
  digitalWrite(kStepPin, LOW);
  digitalWrite(kDirPin, LOW);

  pinMode(kUserButtonPin, INPUT_PULLUP);

  Serial.begin(115200);
  unsigned long serial_wait = 0;
  while (!Serial && serial_wait < 3000UL) {
    delay(10);
    serial_wait += 10;
  }

  print_banner();
}

void hwtest_cl57t_bench_loop() {
  switch (state) {
    case BenchState::WaitForButton:
      if (wait_for_button_press()) {
        Serial.println();
        Serial.println("B1 pressed — starting.");
        state = BenchState::Running;
      }
      break;

    case BenchState::Running:
      // pin_check_square_wave();
      run_motion_sequence();
      state = BenchState::Done;
      break;

    case BenchState::Done:
      delay(1000);
      break;
  }
}

#endif  // HWTEST_CL57T_BENCH
