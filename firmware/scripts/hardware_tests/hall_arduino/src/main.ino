// Hall effect sensor bench test — Arduino Uno / Nano / Mega.
//
// A3144 (and similar) = DIGITAL on/off, not a smooth analog distance sensor.
//   Magnet near  -> OUT LOW  -> "MAGNET NEAR (HOME)"
//   Magnet far   -> OUT HIGH -> "magnet far"
//
// Flash: cd firmware/scripts/hardware_tests && ./run_hall_arduino.sh uno

constexpr uint8_t kHallPin = A0;  // OUT wire from sensor
constexpr uint32_t kPollMs = 100UL;
constexpr uint32_t kStatusMs = 500UL;
constexpr uint32_t kDebounceMs = 15UL;

constexpr int kMagnetNear = LOW;

uint32_t edges = 0;
uint32_t last_status_ms = 0;
bool last_stable = HIGH;

bool ReadDigital() { return digitalRead(kHallPin); }

uint16_t ReadAdc() { return analogRead(kHallPin); }

bool DebouncedState() {
  static bool stable = HIGH;
  static uint32_t last_change = 0;
  const bool sample = ReadDigital();

  if (sample != stable) {
    last_change = millis();
    stable = sample;
  }
  if ((millis() - last_change) >= kDebounceMs) {
    return stable;
  }
  return stable;
}

const char* Label(bool near) { return near ? "MAGNET NEAR (HOME)" : "magnet far"; }

void setup() {
  Serial.begin(115200);
#if defined(USBCON)
  while (!Serial) {
    ;
  }
#endif

  pinMode(kHallPin, INPUT_PULLUP);

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" Arduino Hall test (A3144-style)"));
  Serial.println(F("========================================"));
  Serial.println(F("  OUT -> A0, VCC -> 5V, GND -> GND"));
  Serial.println(F("  Internal pull-up enabled (extra 10k usually OK)."));
  Serial.println(F("  Use digital=LOW/HIGH for homing logic."));
  Serial.println(F("  adc is debug only (expect ~0 or ~1023, not a ramp)."));
  Serial.println();
}

void loop() {
  const bool stable = DebouncedState();
  const bool near = (stable == kMagnetNear);
  const bool digital_now = ReadDigital();
  const uint16_t adc = ReadAdc();

  if (stable != last_stable) {
    last_stable = stable;
    ++edges;
    Serial.print(F("[edge #"));
    Serial.print(edges);
    Serial.print(F("] "));
    Serial.print(Label(near));
    Serial.print(F("  digital="));
    Serial.print(digital_now == LOW ? F("LOW") : F("HIGH"));
    Serial.print(F("  adc="));
    Serial.println(adc);
  }

  if (millis() - last_status_ms >= kStatusMs) {
    last_status_ms = millis();
    Serial.print(F("[status] "));
    Serial.print(Label(near));
    Serial.print(F("  digital="));
    Serial.print(digital_now == LOW ? F("LOW") : F("HIGH"));
    Serial.print(F("  adc="));
    Serial.print(adc);
    Serial.print(F(" (~"));
    Serial.print((adc * 500UL) / 1023UL);
    Serial.print(F(" mV@5V)  edges="));
    Serial.println(edges);
  }

  delay(kPollMs);
}
