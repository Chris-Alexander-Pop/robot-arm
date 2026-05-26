// Hardware test entry point.
//
// This file is built only in the `nucleo_f401re_hwtest` PlatformIO environment.
// The specific test to run is selected via a compile-time flag:
//
//   -DHWTEST_CL57T_BENCH       — slow CL57T bench test on PA0/PA1 (74HCT541 bring-up)
//   -DHWTEST_STEPPER_SINGLE    — single-axis NEMA 23 + CL57T stepper test
//   -DHWTEST_STEPPER_ALL_AXES  — all 6 axes sequential jog
//   -DHWTEST_COMMS             — serial protocol loopback test
//   -DHWTEST_HEARTBEAT         — heartbeat watchdog timeout test
//   -DHWTEST_RS485             — RS-485 transceiver ping (USART1 + DE)
//   -DHWTEST_HALL              — A3144 Hall sensor (active-low on A5)
//
// Each module defines its own setup() / loop() body, prefixed with its
// hwtest name. This file calls the appropriate pair via Arduino setup() and
// loop() so only one test is compiled at a time.
//
// To build and flash a specific test from the command line, use the helper
// scripts in firmware/scripts/hardware_tests/, e.g.:
//
//   ./firmware/scripts/hardware_tests/run_stepper_single.sh

#include <Arduino.h>

// Pull in the selected hardware test implementation.
#include "../test/hardware/hwtest_cl57t_bench.cpp"
#include "../test/hardware/hwtest_stepper_single.cpp"
#include "../test/hardware/hwtest_stepper_all_axes.cpp"
#include "../test/hardware/hwtest_comms.cpp"
#include "../test/hardware/hwtest_heartbeat.cpp"
#include "../test/hardware/hwtest_rs485.cpp"
#include "../test/hardware/hwtest_hall.cpp"

#if !defined(HWTEST_CL57T_BENCH) && \
    !defined(HWTEST_STEPPER_SINGLE) && \
    !defined(HWTEST_STEPPER_ALL_AXES) && \
    !defined(HWTEST_COMMS) && \
    !defined(HWTEST_HEARTBEAT) && \
    !defined(HWTEST_RS485) && \
    !defined(HWTEST_HALL)
#warning "No HWTEST_* flag defined — flash this env with e.g. -DHWTEST_CL57T_BENCH"
#endif

void setup() {
#if defined(HWTEST_CL57T_BENCH)
  hwtest_cl57t_bench_setup();
#elif defined(HWTEST_STEPPER_SINGLE)
  hwtest_stepper_single_setup();
#elif defined(HWTEST_STEPPER_ALL_AXES)
  hwtest_stepper_all_axes_setup();
#elif defined(HWTEST_COMMS)
  hwtest_comms_setup();
#elif defined(HWTEST_HEARTBEAT)
  hwtest_heartbeat_setup();
#elif defined(HWTEST_RS485)
  hwtest_rs485_setup();
#elif defined(HWTEST_HALL)
  hwtest_hall_setup();
#else
  Serial.begin(115200);
  while (!Serial && millis() < 3000UL) { delay(10); }
  Serial.println("No HWTEST_* flag defined. Rebuild with one of:");
  Serial.println("  -DHWTEST_CL57T_BENCH");
  Serial.println("  -DHWTEST_STEPPER_SINGLE");
  Serial.println("  -DHWTEST_STEPPER_ALL_AXES");
  Serial.println("  -DHWTEST_COMMS");
  Serial.println("  -DHWTEST_HEARTBEAT");
  Serial.println("  -DHWTEST_RS485");
  Serial.println("  -DHWTEST_HALL");
#endif
}

void loop() {
#if defined(HWTEST_CL57T_BENCH)
  hwtest_cl57t_bench_loop();
#elif defined(HWTEST_STEPPER_SINGLE)
  hwtest_stepper_single_loop();
#elif defined(HWTEST_STEPPER_ALL_AXES)
  hwtest_stepper_all_axes_loop();
#elif defined(HWTEST_COMMS)
  hwtest_comms_loop();
#elif defined(HWTEST_HEARTBEAT)
  hwtest_heartbeat_loop();
#elif defined(HWTEST_RS485)
  hwtest_rs485_loop();
#elif defined(HWTEST_HALL)
  hwtest_hall_loop();
#else
  delay(1000);
#endif
}
