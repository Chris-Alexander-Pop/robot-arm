#include "motor_driver.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_timer.h>

namespace {

constexpr uint32_t kStepPulseUs = 50U;

int g_step_pin = -1;
int g_dir_pin  = -1;
int g_led_pin  = -1;
uint32_t g_max_hz = 2000U;

esp_timer_handle_t g_timer = nullptr;

volatile bool g_running = false;
volatile bool g_jogging  = false;
volatile uint32_t g_steps_remaining = 0;
volatile uint32_t g_steps_completed = 0;
volatile bool g_done_flag = false;

// GPIO5/6 are MTDI/MTCK on ESP32-C3 — must fully reset IO_MUX before use as GPIO.
void ConfigureOutputPin(int pin, int level) {
    if (pin < 0) {
        return;
    }
    const gpio_num_t gp = static_cast<gpio_num_t>(pin);
    gpio_reset_pin(gp);
    gpio_set_direction(gp, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(gp, GPIO_FLOATING);
    gpio_set_level(gp, level ? 1 : 0);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, level ? HIGH : LOW);
}

void SetPin(int pin, int level) {
    if (pin < 0) {
        return;
    }
    digitalWrite(pin, level ? HIGH : LOW);
    gpio_set_level(static_cast<gpio_num_t>(pin), level ? 1 : 0);
}

void EmitStepPulse() {
    if (g_step_pin < 0) {
        return;
    }
    SetPin(g_step_pin, 1);
    delayMicroseconds(kStepPulseUs);
    SetPin(g_step_pin, 0);
}

void OnStepTimer(void* /*arg*/) {
    if (!g_running) {
        return;
    }

    EmitStepPulse();

    if (g_jogging) {
        ++g_steps_completed;
        return;
    }

    if (g_steps_remaining > 0U) {
        --g_steps_remaining;
        ++g_steps_completed;
    }

    if (!g_jogging && g_steps_remaining == 0U) {
        g_running = false;
        g_done_flag = true;
        if (g_timer != nullptr) {
            esp_timer_stop(g_timer);
        }
        SetPin(g_led_pin, 0);
    }
}

bool StartTimer(uint32_t hz) {
    if (g_timer == nullptr || hz == 0U) {
        return false;
    }
    if (hz > g_max_hz) {
        hz = g_max_hz;
    }
    const uint64_t period_us = 1000000ULL / static_cast<uint64_t>(hz);
    esp_timer_stop(g_timer);
    return esp_timer_start_periodic(g_timer, period_us) == ESP_OK;
}

}  // namespace

void MotorBegin(int step_pin, int dir_pin, int led_pin, uint32_t max_hz) {
    g_step_pin = step_pin;
    g_dir_pin  = dir_pin;
    g_led_pin  = led_pin;
    g_max_hz   = max_hz > 0U ? max_hz : 2000U;

    ConfigureOutputPin(g_step_pin, 0);
    ConfigureOutputPin(g_dir_pin, 0);
    if (g_led_pin >= 0) {
        ConfigureOutputPin(g_led_pin, 0);
    }

    if (g_timer == nullptr) {
        const esp_timer_create_args_t args = {
            .callback = &OnStepTimer,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "motor_step",
            .skip_unhandled_events = true,
        };
        esp_timer_create(&args, &g_timer);
    }
}

bool MotorRun(uint32_t steps, uint8_t dir, uint32_t hz) {
    if (g_timer == nullptr || steps == 0U || hz == 0U) {
        return false;
    }

    MotorStop();

    SetPin(g_dir_pin, dir ? 1 : 0);
    SetPin(g_step_pin, 0);
    g_jogging = false;
    g_steps_remaining = steps;
    g_steps_completed = 0U;
    g_done_flag = false;
    g_running = true;

    SetPin(g_led_pin, 1);
    return StartTimer(hz);
}

bool MotorJog(uint8_t dir, uint32_t hz) {
    if (g_timer == nullptr || hz == 0U) {
        return false;
    }

    MotorStop();

    SetPin(g_dir_pin, dir ? 1 : 0);
    SetPin(g_step_pin, 0);
    g_jogging = true;
    g_steps_remaining = 0U;
    g_steps_completed = 0U;
    g_done_flag = false;
    g_running = true;

    SetPin(g_led_pin, 1);
    return StartTimer(hz);
}

void MotorHold(uint8_t dir) {
    MotorStop();
    // Re-claim pins from any alternate function (JTAG mux on GPIO5/6).
    ConfigureOutputPin(g_step_pin, 0);
    ConfigureOutputPin(g_dir_pin, dir ? 1 : 0);
    SetPin(g_led_pin, dir ? 1 : 0);
}

void MotorStop() {
    g_running = false;
    g_jogging = false;
    g_steps_remaining = 0U;
    g_done_flag = false;

    if (g_timer != nullptr) {
        esp_timer_stop(g_timer);
    }
    SetPin(g_step_pin, 0);
    SetPin(g_dir_pin, 0);  // leave both lines idle after STOP
    SetPin(g_led_pin, 0);
}

bool MotorIsRunning() {
    return g_running;
}

uint32_t MotorStepsCompleted() {
    return g_steps_completed;
}

bool MotorPollDone() {
    if (!g_done_flag) {
        return false;
    }
    g_done_flag = false;
    return true;
}

int MotorReadStepLevel() {
    if (g_step_pin < 0) {
        return -1;
    }
    return digitalRead(g_step_pin);
}

int MotorReadDirLevel() {
    if (g_dir_pin < 0) {
        return -1;
    }
    return digitalRead(g_dir_pin);
}
