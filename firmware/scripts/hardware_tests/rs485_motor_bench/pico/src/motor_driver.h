#pragma once

#include <stdint.h>
#include <stdbool.h>

// Non-blocking STEP/DIR pulse generator for a motor wired directly to the Pico.
// Call MotorTick() from loop(); safe to call MotorStop() at any time.

void MotorBegin(int step_pin, int dir_pin, int led_pin, uint32_t max_hz);

bool MotorRun(uint32_t steps, uint8_t dir, uint32_t hz);
bool MotorJog(uint8_t dir, uint32_t hz);
void MotorHold(uint8_t dir);  // steady DIR (multimeter); STEP low
void MotorStop();
void MotorTick();  // advance pending step pulses

bool MotorIsRunning();
uint32_t MotorStepsCompleted();
