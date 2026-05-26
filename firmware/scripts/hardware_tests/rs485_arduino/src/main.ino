// RS-485 bus partner for bench testing (Arduino Uno/Nano or Mega).
//
// PlatformIO: firmware/scripts/hardware_tests/rs485_arduino/
// Baud / framing: ../rs485_bench_config.h (must match STM32 hwtest_rs485.cpp)

#include "rs485_bench_config.h"

#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#define RS485_USE_SERIAL1
constexpr uint8_t kRs485DePin = 22;
#else
// AltSoftSerial on Uno/Nano: fixed pins 8 (RX) and 9 (TX) — timer-driven, not bit-bang.
#include <AltSoftSerial.h>
constexpr uint8_t kRs485DePin = 7;
AltSoftSerial rs485Alt;
#endif

constexpr uint32_t kRs485Baud = RS485_BENCH_BAUD;

uint32_t frames_ok = 0;
uint32_t frames_bad = 0;
uint32_t echoes_sent = 0;

void SetTransmitMode(bool transmit) {
  digitalWrite(kRs485DePin, transmit ? HIGH : LOW);
  if (transmit) {
    Rs485().flush();
    delayMicroseconds(RS485_BENCH_DE_SETTLE_US);
  } else {
    delayMicroseconds(RS485_BENCH_DE_SETTLE_US);
  }
}

Stream& Rs485() {
#ifdef RS485_USE_SERIAL1
  return Serial1;
#else
  return rs485Alt;
#endif
}

void Rs485Begin() {
#ifdef RS485_USE_SERIAL1
  Serial1.begin(kRs485Baud);
#else
  rs485Alt.begin(kRs485Baud);
#endif
}

void DrainRs485Rx() {
  uint32_t quiet_ms = 0;
  while (quiet_ms < RS485_BENCH_FRAME_GAP_MS) {
    if (Rs485().available() > 0) {
      while (Rs485().available() > 0) {
        Rs485().read();
      }
      quiet_ms = 0;
    } else {
      delay(1);
      ++quiet_ms;
    }
  }
}

size_t ReadRs485Frame(uint8_t* out, size_t capacity) {
  if (Rs485().available() == 0) {
    return 0;
  }

  size_t count = 0;
  uint32_t last_byte_ms = millis();

  while (count < capacity) {
    if (Rs485().available() > 0) {
      const int byte = Rs485().read();
      if (byte < 0) {
        break;
      }
      out[count++] = static_cast<uint8_t>(byte);
      last_byte_ms = millis();
    } else if (count > 0 && (millis() - last_byte_ms) >= RS485_BENCH_FRAME_GAP_MS) {
      break;
    } else if (count == 0) {
      return 0;
    }
  }
  return count;
}

bool IsExactPing(const uint8_t* data, size_t length) {
  return length == RS485_BENCH_PING_LEN &&
         memcmp(data, RS485_BENCH_PING, RS485_BENCH_PING_LEN) == 0;
}

bool IsPlausiblePingFrame(const uint8_t* data, size_t length) {
  if (length < 5 || length > 32) {
    return false;
  }
  if (memcmp(data, "RS485", 5) != 0) {
    return false;
  }
  for (size_t i = 0; i < length; ++i) {
    const uint8_t b = data[i];
    if (b == '\r' || b == '\n' || b == ' ') {
      continue;
    }
    if (b < 0x21 || b > 0x7E) {
      return false;
    }
  }
  return true;
}

void PrintHex(const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    if (data[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
    if (i + 1 < length) {
      Serial.print(' ');
    }
  }
}

void PrintAscii(const uint8_t* data, size_t length) {
  Serial.print('"');
  for (size_t i = 0; i < length; ++i) {
    const char c = static_cast<char>(data[i]);
    Serial.print((c >= 32 && c < 127) ? c : '.');
  }
  Serial.print('"');
}

void EchoFrame(const uint8_t* data, size_t length) {
  DrainRs485Rx();
  SetTransmitMode(true);
  const size_t written = Rs485().write(data, length);
  Rs485().flush();
  SetTransmitMode(false);

  ++echoes_sent;
  Serial.print(F("  TX echo ("));
  Serial.print(written);
  Serial.print(F(" bytes) hex "));
  PrintHex(data, length);
  Serial.print(F(" ascii "));
  PrintAscii(data, length);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
#if defined(USBCON)
  while (!Serial) {
    ;
  }
#endif

  pinMode(kRs485DePin, OUTPUT);
  SetTransmitMode(false);
  Rs485Begin();
  DrainRs485Rx();

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" Arduino RS-485 echo (bus partner)"));
  Serial.println(F("========================================"));
  Serial.print(F("  Bus baud: "));
  Serial.println(kRs485Baud);
#ifdef RS485_USE_SERIAL1
  Serial.println(F("  UART: Mega Serial1 (18/19), DE=D22"));
#else
  Serial.println(F("  UART: AltSoftSerial (D8=RX D9=TX), DE=D7"));
#endif
  Serial.println(F("  Echoes only valid RS485* frames (drops garbage)."));
  Serial.println(F("  Pair with: ./run_rs485.sh on Nucleo"));
  Serial.println();
}

void loop() {
  uint8_t buf[64];
  const size_t n = ReadRs485Frame(buf, sizeof(buf));
  if (n == 0) {
    return;
  }

  const bool exact = IsExactPing(buf, n);
  const bool plausible = IsPlausiblePingFrame(buf, n);

  if (exact) {
    ++frames_ok;
  } else {
    ++frames_bad;
  }

  Serial.print(F("RX ("));
  Serial.print(n);
  Serial.print(exact ? F(" OK") : (plausible ? F(" ~ok") : F(" BAD")));
  Serial.print(F(") ok="));
  Serial.print(frames_ok);
  Serial.print(F(" bad="));
  Serial.print(frames_bad);
  Serial.print(F(" hex "));
  PrintHex(buf, n);
  Serial.print(F(" ascii "));
  PrintAscii(buf, n);
  Serial.println();

  if (plausible) {
    EchoFrame(buf, n);
  } else {
    Serial.println(F("  (discarded — not echoed)"));
    DrainRs485Rx();
  }
}
