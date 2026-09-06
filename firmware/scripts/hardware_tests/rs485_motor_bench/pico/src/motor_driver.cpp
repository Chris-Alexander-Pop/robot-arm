#include "motor_driver.h"

#include <Arduino.h>

namespace {

constexpr uint32_t kStepPulseUs = 50U;

int g_step_pin = -1;
int g_dir_pin  = -1;
int g_led_pin  = -1;
uint32_t g_max_hz = 3000U;

bool g_running = false;
bool g_jogging = false;
uint32_t g_steps_remaining = 0U;
uint32_t g_steps_completed = 0U;
uint32_t g_period_us = 0U;
uint32_t g_next_step_us = 0U;

void SetPin(int pin, int level) {
    if (pin < 0) {
        return;
    }
    digitalWrite(pin, level ? HIGH : LOW);
}

void EmitStepPulse() {
    if (g_step_pin < 0) {
        return;
    }
    SetPin(g_step_pin, 1);
    delayMicroseconds(kStepPulseUs);
    SetPin(g_step_pin, 0);
}

}  // namespace

void MotorBegin(int step_pin, int dir_pin, int led_pin, uint32_t max_hz) {
    g_step_pin = step_pin;
    g_dir_pin  = dir_pin;
    g_led_pin  = led_pin;
    g_max_hz   = max_hz > 0U ? max_hz : 3000U;

    pinMode(g_step_pin, OUTPUT);
    pinMode(g_dir_pin, OUTPUT);
    SetPin(g_step_pin, 0);
    SetPin(g_dir_pin, 0);
    if (g_led_pin >= 0) {
        pinMode(g_led_pin, OUTPUT);
        SetPin(g_led_pin, 0);
    }
}

bool MotorRun(uint32_t steps, uint8_t dir, uint32_t hz) {
    if (steps == 0U || hz == 0U || g_step_pin < 0) {
        return false;
    }

    MotorStop();

    if (hz > g_max_hz) {
        hz = g_max_hz;
    }

    SetPin(g_dir_pin, dir ? 1 : 0);
    SetPin(g_step_pin, 0);
    g_jogging = false;
    g_steps_remaining = steps;
    g_steps_completed = 0U;
    g_period_us = 1000000U / hz;
    g_next_step_us = micros();
    g_running = true;
    SetPin(g_led_pin, 1);
    return true;
}

bool MotorJog(uint8_t dir, uint32_t hz) {
    if (hz == 0U || g_step_pin < 0) {
        return false;
    }

    MotorStop();

    if (hz > g_max_hz) {
        hz = g_max_hz;
    }

    SetPin(g_dir_pin, dir ? 1 : 0);
    SetPin(g_step_pin, 0);
    g_jogging = true;
    g_steps_remaining = 0U;
    g_steps_completed = 0U;
    g_period_us = 1000000U / hz;
    g_next_step_us = micros();
    g_running = true;
    SetPin(g_led_pin, 1);
    return true;
}

void MotorHold(uint8_t dir) {
    MotorStop();
    SetPin(g_step_pin, 0);
    SetPin(g_dir_pin, dir ? 1 : 0);
    SetPin(g_led_pin, dir ? 1 : 0);
}

void MotorStop() {
    g_running = false;
    g_jogging = false;
    g_steps_remaining = 0U;
    g_period_us = 0U;
    SetPin(g_step_pin, 0);
    SetPin(g_dir_pin, 0);
    SetPin(g_led_pin, 0);
}

void MotorTick() {
    if (!g_running || g_period_us == 0U) {
        return;
    }

    const uint32_t now = micros();
    // Signed compare so micros() wrap is handled.
    if (static_cast<int32_t>(now - g_next_step_us) < 0) {
        return;
    }

    EmitStepPulse();
    g_next_step_us += g_period_us;

    if (g_jogging) {
        ++g_steps_completed;
        return;
    }

    if (g_steps_remaining > 0U) {
        --g_steps_remaining;
        ++g_steps_completed;
    }

    if (g_steps_remaining == 0U) {
        g_running = false;
        SetPin(g_led_pin, 0);
    }
}

bool MotorIsRunning() {
    return g_running;
}

uint32_t MotorStepsCompleted() {
    return g_steps_completed;
}
