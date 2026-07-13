#pragma once

#include <stdint.h>
#include <stdbool.h>

// Non-blocking STEP/DIR pulse generator for CL57T / CL42T drivers.
// Uses esp_timer periodic callback; safe to call MotorStop() from RS-485 loop.

void MotorBegin(int step_pin, int dir_pin, int led_pin, uint32_t max_hz);

bool MotorRun(uint32_t steps, uint8_t dir, uint32_t hz);
bool MotorJog(uint8_t dir, uint32_t hz);
void MotorHold(uint8_t dir);  // steady DIR for multimeter; STEP low
void MotorStop();

bool MotorIsRunning();
uint32_t MotorStepsCompleted();
bool MotorPollDone();  // true once when a RUN completes; clears flag

// Readback of driven levels (for HOLD diagnostics over WiFi).
int MotorReadStepLevel();
int MotorReadDirLevel();
