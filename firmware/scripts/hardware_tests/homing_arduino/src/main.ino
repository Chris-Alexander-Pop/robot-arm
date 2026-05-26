// Homing bench test — Arduino Uno + CL57T + A3144 Hall.
//
// Motor pattern from firmware/stm32_core/test/hardware/hwtest_cl57t_bench.cpp
// Homing FSM from docs/implementation/distributed_bus_architecture.md
//
// Flash: cd firmware/scripts/hardware_tests && ./run_homing_arduino.sh uno

// --- Pins (Uno) — change here if your bench differs ---
constexpr uint8_t kHallPin = A0;
constexpr uint8_t kStepPin = 2;   // D2 -> CL57T PUL+ (via 74HCT541 if used)
constexpr uint8_t kDirPin = 3;    // D3 -> CL57T DIR+
constexpr uint8_t kEnablePin = 4;  // D4 -> CL57T ENA+ (active-low enable)
constexpr uint8_t kAlarmPin = 5;     // D5 <- CL57T ALM+ (optional)

// CL57T DIP microsteps: 200 full steps * 8 = 1600 steps/rev (adjust to your driver).
constexpr float kStepsPerRev = 1600.0F;
// Bench speed 10 from cl57t_bench_arduino / hwtest_cl57t_bench (1500 * 3 = 4500 steps/s).
constexpr float kHomingStepsPerSec = 4500.0F;
constexpr unsigned int kStepPulseUs = 50U;
constexpr uint32_t kMaxSeekSteps = 80000UL;  // safety stop if Hall never seen
constexpr uint16_t kBackoffSteps = 80U;      // step away from Hall after trigger

// true = DIR HIGH while seeking home. Flip if motor runs the wrong way.
constexpr bool kSeekHomeDirHigh = true;

// A3144: LOW when magnet at home position.
constexpr int kMagnetNearLevel = LOW;

enum class State { Idle, SeekingHome, BackingOff, Homed, Fault };

State state = State::Idle;
long position_steps = 0;
bool homed = false;
uint32_t seek_steps = 0;

void EmitStepPulse() {
  digitalWrite(kStepPin, HIGH);
  delayMicroseconds(kStepPulseUs);
  digitalWrite(kStepPin, LOW);
}

bool HallMagnetNear() { return digitalRead(kHallPin) == kMagnetNearLevel; }

bool DriverAlarmActive() { return digitalRead(kAlarmPin) == LOW; }

void SetDriverEnabled(bool enable) {
  // CL57T ENA is typically active-low: LOW = motor enabled.
  digitalWrite(kEnablePin, enable ? LOW : HIGH);
}

void SetSeekDirection() {
  digitalWrite(kDirPin, kSeekHomeDirHigh ? HIGH : LOW);
}

void SetBackoffDirection() {
  digitalWrite(kDirPin, kSeekHomeDirHigh ? LOW : HIGH);
}

bool StepOnce(float steps_per_sec) {
  if (steps_per_sec <= 0.0F) {
    return false;
  }
  const unsigned long interval_us = static_cast<unsigned long>(1000000.0F / steps_per_sec);
  static unsigned long last_us = 0;
  const unsigned long now = micros();
  if (now - last_us < interval_us) {
    return false;
  }
  last_us = now;
  EmitStepPulse();
  if (digitalRead(kDirPin) == (kSeekHomeDirHigh ? HIGH : LOW)) {
    ++position_steps;
  } else {
    --position_steps;
  }
  return true;
}

void EnterFault(const __FlashStringHelper* reason) {
  state = State::Fault;
  homed = false;
  SetDriverEnabled(false);
  digitalWrite(kStepPin, LOW);
  Serial.print(F("FAULT: "));
  Serial.println(reason);
}

void PrintBanner() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" Arduino homing test (CL57T + Hall)"));
  Serial.println(F("========================================"));
  Serial.println(F("  Hall A0   STEP D2   DIR D3   EN D4   ALM D5"));
  Serial.println(F("  Type HOME in serial monitor to start seek."));
  Serial.println(F("  CL57T motor encoder -> driver P2 only (NOT Arduino)."));
  Serial.println();
}

void setup() {
  Serial.begin(115200);
#if defined(USBCON)
  while (!Serial) {
    ;
  }
#endif

  pinMode(kHallPin, INPUT_PULLUP);
  pinMode(kStepPin, OUTPUT);
  pinMode(kDirPin, OUTPUT);
  pinMode(kEnablePin, OUTPUT);
  pinMode(kAlarmPin, INPUT_PULLUP);

  digitalWrite(kStepPin, LOW);
  digitalWrite(kDirPin, LOW);
  SetDriverEnabled(false);

  PrintBanner();
  Serial.print(F("  Hall now: "));
  Serial.println(HallMagnetNear() ? F("MAGNET NEAR") : F("far"));
  if (DriverAlarmActive()) {
    EnterFault(F("ALM active at startup"));
  }
}

void StartHoming() {
  if (state == State::SeekingHome) {
    return;
  }
  if (DriverAlarmActive()) {
    EnterFault(F("ALM active before HOME"));
    return;
  }

  Serial.println();
  Serial.println(F("HOME — enabling driver, seeking Hall..."));
  position_steps = 0;
  seek_steps = 0;
  homed = false;
  SetDriverEnabled(true);
  SetSeekDirection();
  state = State::SeekingHome;
}

void LoopHoming() {
  if (DriverAlarmActive()) {
    EnterFault(F("ALM during homing"));
    return;
  }

  if (HallMagnetNear()) {
    Serial.print(F("  Hall triggered at step "));
    Serial.println(seek_steps);
    SetDriverEnabled(false);
    state = State::BackingOff;
    Serial.println(F("  Backing off a few steps..."));
    SetBackoffDirection();
    SetDriverEnabled(true);
    return;
  }

  if (seek_steps >= kMaxSeekSteps) {
    EnterFault(F("max seek steps — Hall not seen (wrong DIR? magnet?)"));
    return;
  }

  if (StepOnce(kHomingStepsPerSec)) {
    ++seek_steps;
  }
}

void LoopBackoff() {
  static uint16_t done = 0;
  if (done < kBackoffSteps) {
    if (StepOnce(kHomingStepsPerSec)) {
      ++done;
    }
    return;
  }

  done = 0;
  SetDriverEnabled(false);
  position_steps = 0;
  homed = true;
  state = State::Homed;
  Serial.println();
  Serial.println(F("HOMED OK — position_steps = 0"));
  Serial.print(F("  Hall: "));
  Serial.println(HallMagnetNear() ? F("near") : F("far"));
}

void HandleSerial() {
  if (!Serial.available()) {
    return;
  }
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == F("HOME")) {
    state = State::Idle;
    StartHoming();
  } else if (cmd == F("STATUS")) {
    Serial.print(F("state="));
    switch (state) {
      case State::Idle: Serial.print(F("idle")); break;
      case State::SeekingHome: Serial.print(F("seeking")); break;
      case State::BackingOff: Serial.print(F("backoff")); break;
      case State::Homed: Serial.print(F("homed")); break;
      case State::Fault: Serial.print(F("fault")); break;
    }
    Serial.print(F("  homed="));
    Serial.print(homed ? 1 : 0);
    Serial.print(F("  pos_steps="));
    Serial.print(position_steps);
    Serial.print(F("  hall="));
    Serial.println(HallMagnetNear() ? F("NEAR") : F("far"));
  } else if (cmd == F("RESET")) {
    state = State::Idle;
    homed = false;
    SetDriverEnabled(false);
    Serial.println(F("Reset to idle."));
  } else {
    Serial.println(F("Commands: HOME | STATUS | RESET"));
  }
}

void loop() {
  HandleSerial();

  switch (state) {
    case State::Idle:
    case State::Homed:
    case State::Fault:
      break;
    case State::SeekingHome:
      LoopHoming();
      break;
    case State::BackingOff:
      LoopBackoff();
      break;
  }
}
