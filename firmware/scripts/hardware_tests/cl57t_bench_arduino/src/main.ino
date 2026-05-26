// CL57T bench test for Arduino Uno — port of stm32_core/test/hardware/hwtest_cl57t_bench.cpp
//
// Creep, then speeds 1..12: each speed runs forward then reverse (2 s dwell each).
// Type GO in serial monitor to start (Nucleo version used the blue USER button).
//
// Flash: ../run_cl57t_bench_arduino.sh uno

constexpr uint8_t kStepPin = 2;     // D2 -> PUL+
constexpr uint8_t kDirPin = 3;      // D3 -> DIR+
constexpr uint8_t kEnablePin = 4;   // D4 -> ENA+ (active-low enable)

constexpr float kStepsPerRev = 1600.0F;
constexpr unsigned long kDwellMs = 2000UL;
constexpr unsigned int kStepPulseUs = 50U;

constexpr float kSpeed8StepS = 1500.0F;
constexpr float kSpeeds[] = {
    50.0F, 100.0F, 200.0F, 350.0F, 500.0F, 750.0F, 1000.0F, kSpeed8StepS,
    kSpeed8StepS * 2.0F,
    kSpeed8StepS * 3.0F,
    kSpeed8StepS * 4.0F,
    kSpeed8StepS * 5.0F,
};
constexpr const char* kSpeedLabels[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12",
};
constexpr size_t kSpeedCount = sizeof(kSpeeds) / sizeof(kSpeeds[0]);

enum class BenchState { WaitForGo, Running, Done };

BenchState state = BenchState::WaitForGo;

void SetDriverEnabled(bool enable) {
  digitalWrite(kEnablePin, enable ? LOW : HIGH);
}

void PrintBanner() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" Arduino CL57T bench (D2=STEP D3=DIR)"));
  Serial.println(F("========================================"));
  Serial.print(F("  Steps/rev (DIP): "));
  Serial.println(static_cast<int>(kStepsPerRev));
  Serial.print(F("  Speed range: "));
  Serial.print(kSpeeds[0], 0);
  Serial.print(F(" .. "));
  Serial.print(kSpeeds[kSpeedCount - 1], 0);
  Serial.println(F(" steps/s"));
  Serial.println(F("  Each speed: 2 s forward, 2 s reverse."));
  Serial.println(F("  >>> Type GO in serial monitor to start <<<"));
  Serial.println(F("========================================"));
}

void EmitStepPulse() {
  digitalWrite(kStepPin, HIGH);
  delayMicroseconds(kStepPulseUs);
  digitalWrite(kStepPin, LOW);
}

unsigned long RunVisibleCreep(float steps_per_sec, unsigned long duration_ms, bool forward,
                            unsigned int pulse_ms) {
  if (steps_per_sec <= 0.0F) {
    return 0;
  }
  digitalWrite(kDirPin, forward ? HIGH : LOW);
  const unsigned long interval_ms = static_cast<unsigned long>(1000.0F / steps_per_sec);
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

unsigned long RunAtSpeed(float steps_per_sec, unsigned long duration_ms, bool forward) {
  if (steps_per_sec <= 0.0F) {
    return 0;
  }
  digitalWrite(kDirPin, forward ? HIGH : LOW);
  const unsigned long interval_us = static_cast<unsigned long>(1000000.0F / steps_per_sec);
  unsigned long last_pulse_us = micros();
  const unsigned long t_end = millis() + duration_ms;
  unsigned long count = 0;

  while (millis() < t_end) {
    const unsigned long now = micros();
    if (now - last_pulse_us >= interval_us) {
      EmitStepPulse();
      last_pulse_us = now;
      ++count;
    }
  }
  return count;
}

void CoastToStop() {
  digitalWrite(kStepPin, LOW);
  delay(200);
}

void RunPhase(const char* label, float speed_step_s, bool forward) {
  Serial.print(F("  "));
  Serial.print(label);
  Serial.print(F(" @ "));
  Serial.print(speed_step_s, 0);
  Serial.print(F(" steps/s ... "));
  const unsigned long n = RunAtSpeed(speed_step_s, kDwellMs, forward);
  CoastToStop();
  Serial.print(F("OK ("));
  Serial.print(n);
  Serial.println(F(" pulses)"));
  delay(300);
}

void RunSpeedSweepAlternating() {
  for (size_t i = 0; i < kSpeedCount; ++i) {
    char label[20];
    snprintf(label, sizeof(label), "speed %s fwd", kSpeedLabels[i]);
    RunPhase(label, kSpeeds[i], true);
    snprintf(label, sizeof(label), "speed %s rev", kSpeedLabels[i]);
    RunPhase(label, kSpeeds[i], false);
  }
}

void RunMotionSequence() {
  SetDriverEnabled(true);

  Serial.println();
  Serial.println(F("GO — creep, then speed 1..12 (fwd/rev pairs)."));
  Serial.println(F("----------------------------------------"));

  Serial.println(F("[0] Creep forward (1 step/s, 5 s)"));
  const unsigned long creep_n = RunVisibleCreep(1.0F, 5000UL, true, 200U);
  CoastToStop();
  Serial.print(F("    creep pulses: "));
  Serial.println(creep_n);
  delay(400);

  Serial.println(F("[1] Speeds 1..12 (forward, then reverse each)"));
  RunSpeedSweepAlternating();

  digitalWrite(kStepPin, LOW);
  digitalWrite(kDirPin, LOW);
  SetDriverEnabled(false);

  Serial.println(F("----------------------------------------"));
  Serial.println(F("BENCH COMPLETE"));
  Serial.println(F("----------------------------------------"));
}

void HandleSerial() {
  if (!Serial.available()) {
    return;
  }
  String line = Serial.readStringUntil('\n');
  line.trim();
  line.toUpperCase();
  if (line == F("GO") && state == BenchState::WaitForGo) {
    Serial.println();
    Serial.println(F("GO received — starting."));
    state = BenchState::Running;
  } else if (line == F("GO") && state == BenchState::Done) {
    Serial.println(F("Already done. Reset board or add RESET command."));
  }
}

void setup() {
  Serial.begin(115200);
#if defined(USBCON)
  while (!Serial) {
    ;
  }
#endif

  pinMode(kStepPin, OUTPUT);
  pinMode(kDirPin, OUTPUT);
  pinMode(kEnablePin, OUTPUT);
  digitalWrite(kStepPin, LOW);
  digitalWrite(kDirPin, LOW);
  SetDriverEnabled(false);

  PrintBanner();
}

void loop() {
  HandleSerial();

  switch (state) {
    case BenchState::WaitForGo:
      break;
    case BenchState::Running:
      RunMotionSequence();
      state = BenchState::Done;
      break;
    case BenchState::Done:
      delay(500);
      break;
  }
}
