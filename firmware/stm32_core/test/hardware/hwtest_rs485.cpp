// Hardware test: RS-485 transceiver (MAX485 / MAX3485) on USART1.
//
// Bench settings: firmware/scripts/hardware_tests/rs485_bench_config.h
// Partner: rs485_arduino (AltSoftSerial @ 38400 on Uno)

#ifdef HWTEST_RS485

#include <Arduino.h>
#include <cstring>

#include "pinout.h"
#include "rs485_bench_config.h"

namespace {

HardwareSerial& Rs485Uart() {
  static HardwareSerial serial(kRs485RxPin, kRs485TxPin);
  return serial;
}

constexpr uint32_t kRs485Baud = RS485_BENCH_BAUD;
constexpr uint32_t kPingIntervalMs = 1000U;
constexpr uint32_t kRxWindowMs = 80U;

uint32_t last_ping_ms = 0U;
uint32_t pings_sent = 0U;
uint32_t replies_ok = 0U;
uint32_t replies_bad = 0U;

void SetTransmitMode(bool transmit) {
  digitalWrite(kRs485DePin, transmit ? HIGH : LOW);
  if (transmit) {
    Rs485Uart().flush();
    delayMicroseconds(RS485_BENCH_DE_SETTLE_US);
  } else {
    delayMicroseconds(RS485_BENCH_DE_SETTLE_US);
  }
}

void DrainRs485Rx() {
  const uint32_t deadline = millis() + RS485_BENCH_FRAME_GAP_MS;
  while (millis() < deadline) {
    while (Rs485Uart().available() > 0) {
      Rs485Uart().read();
    }
    delay(1);
  }
}

size_t ReadRs485Frame(uint8_t* out, size_t capacity, uint32_t timeout_ms) {
  const uint32_t deadline = millis() + timeout_ms;
  size_t count = 0;
  uint32_t last_byte_ms = 0;

  while (millis() < deadline && count < capacity) {
    if (Rs485Uart().available() > 0) {
      const int byte = Rs485Uart().read();
      if (byte < 0) {
        break;
      }
      out[count++] = static_cast<uint8_t>(byte);
      last_byte_ms = millis();
    } else if (count > 0 && (millis() - last_byte_ms) >= RS485_BENCH_FRAME_GAP_MS) {
      break;
    }
    delay(1);
  }
  return count;
}

bool IsExactPing(const uint8_t* data, size_t length) {
  return length == RS485_BENCH_PING_LEN &&
         memcmp(data, RS485_BENCH_PING, RS485_BENCH_PING_LEN) == 0;
}

void PrintHexLine(const uint8_t* data, size_t length) {
  for (size_t i = 0U; i < length; ++i) {
    if (data[i] < 0x10U) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
    if (i + 1U < length) {
      Serial.print(' ');
    }
  }
}

}  // namespace

void hwtest_rs485_setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000UL) {
    delay(10);
  }

  pinMode(kRs485DePin, OUTPUT);
  SetTransmitMode(false);
  Rs485Uart().begin(kRs485Baud);
  DrainRs485Rx();

  Serial.println();
  Serial.println("========================================");
  Serial.println(" HWTEST: RS-485 (USART1, Nucleo D8/D2/A2)");
  Serial.println("========================================");
  Serial.print("  Bus baud: ");
  Serial.println(kRs485Baud);
  Serial.println("  Pins: D8=TX, D2=RX, A2=DE (3.3V + GND)");
  Serial.println("  Ping once per second; counts exact echo matches.");
  Serial.println();
}

void hwtest_rs485_loop() {
  const uint32_t now = millis();

  if (now - last_ping_ms < kPingIntervalMs) {
    return;
  }
  last_ping_ms = now;

  DrainRs485Rx();

  static const char kPing[] = RS485_BENCH_PING;
  const size_t ping_len = RS485_BENCH_PING_LEN;

  SetTransmitMode(true);
  const size_t written = Rs485Uart().write(reinterpret_cast<const uint8_t*>(kPing), ping_len);
  Rs485Uart().flush();
  SetTransmitMode(false);

  ++pings_sent;
  Serial.print("[#");
  Serial.print(pings_sent);
  Serial.print("] TX (");
  Serial.print(written);
  Serial.print(" bytes): ");
  Serial.println(kPing);

  uint8_t rx_buf[64]{};
  const size_t rx_len = ReadRs485Frame(rx_buf, sizeof(rx_buf), kRxWindowMs);

  if (rx_len == 0U) {
    Serial.println("     RX: (none)");
    return;
  }

  const bool exact = IsExactPing(rx_buf, rx_len);
  if (exact) {
    ++replies_ok;
  } else {
    ++replies_bad;
  }

  Serial.print("     RX (");
  Serial.print(rx_len);
  Serial.print(exact ? " OK" : " BAD");
  Serial.print(", ok=");
  Serial.print(replies_ok);
  Serial.print(" bad=");
  Serial.print(replies_bad);
  Serial.print("): hex ");
  PrintHexLine(rx_buf, rx_len);
  Serial.print("  ascii \"");
  for (size_t i = 0U; i < rx_len; ++i) {
    const char c = static_cast<char>(rx_buf[i]);
    Serial.print((c >= 32 && c < 127) ? c : '.');
  }
  Serial.println('"');
}

#endif  // HWTEST_RS485
