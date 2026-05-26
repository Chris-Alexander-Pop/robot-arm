// Hardware test: Hall effect sensor (A3144-style, active-low digital output).
//
// The A3144 is a DIGITAL switch (on/off), not a linear analog sensor.
// This test prints:
//   - digital: HIGH/LOW (what homing firmware uses)
//   - adc: 0-4095 (ADC on the same pin — expect ~0 or ~4095 only, not a smooth range)
//
// Wiring: OUT -> Nucleo A5 (PC0), VCC -> 3.3V, GND -> GND
// Pull-up: firmware INPUT_PULLUP (external 10k optional for bare chip — see README)
//
// Flash: cd firmware/scripts/hardware_tests && ./run_hall.sh

#ifdef HWTEST_HALL

#include <Arduino.h>

#include "pinout.h"

namespace {

// Same physical pin as kHallBenchPin (PC0) on the Arduino header.
constexpr uint8_t kHallAnalogInput = A5;

constexpr uint32_t kPollIntervalMs = 100U;
constexpr uint32_t kDebounceMs = 15U;
constexpr uint32_t kStatusPrintMs = 500U;

constexpr int kMagnetNearLevel = LOW;

bool debounced_state = HIGH;
bool last_reported_state = HIGH;
uint32_t trigger_count = 0U;
uint32_t last_status_ms = 0U;

bool ReadHallDigital() { return digitalRead(kHallBenchPin); }

uint16_t ReadHallAdc() { return analogRead(kHallAnalogInput); }

bool DebouncedHallState() {
  const bool sample = ReadHallDigital();
  static bool stable = HIGH;
  static uint32_t last_change_ms = 0U;

  if (sample != stable) {
    last_change_ms = millis();
    stable = sample;
  }

  if ((millis() - last_change_ms) >= kDebounceMs) {
    debounced_state = stable;
  }
  return debounced_state;
}

const char* StateLabel(bool magnet_near) {
  return magnet_near ? "MAGNET NEAR (HOME)" : "magnet far";
}

}  // namespace

void hwtest_hall_setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000UL) {
    delay(10);
  }

  pinMode(kHallBenchPin, INPUT_PULLUP);

  debounced_state = ReadHallDigital();
  last_reported_state = debounced_state;
  last_status_ms = millis();

  Serial.println();
  Serial.println("========================================");
  Serial.println(" HWTEST: Hall sensor (A3144-style)");
  Serial.println("========================================");
  Serial.println("  A3144 = DIGITAL on/off (not distance analog).");
  Serial.println("  Pin: Nucleo A5 (PC0) + internal pull-up.");
  Serial.println("  Magnet near -> digital LOW.");
  Serial.println();
  Serial.println("  External 10k pull-up (VCC->OUT):");
  Serial.println("    NOT required if this test reads HIGH/LOW reliably.");
  Serial.println("    Add one (to 3.3V) if stuck LOW or floating.");
  Serial.println();
  Serial.println("  adc=0..4095 is optional debug; expect only near 0 OR near 4095.");
  Serial.println();
  Serial.print("  Initial: ");
  Serial.print(StateLabel(debounced_state == kMagnetNearLevel));
  Serial.print("  digital=");
  Serial.print(ReadHallDigital() == LOW ? "LOW" : "HIGH");
  Serial.print("  adc=");
  Serial.println(ReadHallAdc());
  Serial.println();
}

void hwtest_hall_loop() {
  const bool state = DebouncedHallState();
  const bool magnet_near = (state == kMagnetNearLevel);
  const bool digital_raw = ReadHallDigital();
  const uint16_t adc = ReadHallAdc();

  if (state != last_reported_state) {
    last_reported_state = state;
    ++trigger_count;
    Serial.print("[edge #");
    Serial.print(trigger_count);
    Serial.print("] ");
    Serial.print(StateLabel(magnet_near));
    Serial.print("  digital=");
    Serial.print(digital_raw == LOW ? "LOW" : "HIGH");
    Serial.print("  adc=");
    Serial.println(adc);
  }

  const uint32_t now = millis();
  if (now - last_status_ms >= kStatusPrintMs) {
    last_status_ms = now;
    Serial.print("[status] ");
    Serial.print(StateLabel(magnet_near));
    Serial.print("  digital=");
    Serial.print(digital_raw == LOW ? "LOW" : "HIGH");
    Serial.print("  adc=");
    Serial.print(adc);
    Serial.print("  (~");
    Serial.print((adc * 3300UL) / 4095UL);
    Serial.print(" mV)  triggers=");
    Serial.println(trigger_count);
  }

  delay(kPollIntervalMs);
}

#endif  // HWTEST_HALL
