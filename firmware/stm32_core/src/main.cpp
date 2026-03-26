#include <Arduino.h>
#include <AccelStepper.h>
#include <Wire.h>
#include <AS5600.h>

// --- Pin Definitions ---
// Uses AccelStepper::DRIVER (1 pin for step, 1 for dir)
#define MOTOR1_STEP_PIN 2
#define MOTOR1_DIR_PIN  3

// --- Motor & Encoder Objects ---
AccelStepper stepper1(AccelStepper::DRIVER, MOTOR1_STEP_PIN, MOTOR1_DIR_PIN);
AS5600 encoder1(&Wire);

// --- PID control variables ---
float target_angle = 0.0;
float current_angle = 0.0;
float kp = 10.0; // Proportional gain placeholder

void setup() {
  Serial.begin(115200);
  const unsigned long serial_wait_start_ms = millis();
  while (!Serial && (millis() - serial_wait_start_ms) < 2000UL) {
    delay(10);
  }
  Serial.println("STM32 Robot Arm Controller Initializing...");
  
  // Initialize I2C for AS5600 encoders (Uses standard SDA/SCL pins)
  Wire.begin();
  encoder1.begin();
  
  // Setup Motor Profile
  stepper1.setMaxSpeed(2000.0);
  stepper1.setAcceleration(500.0);
}

void loop() {
  /*
   * MAIN CONTROL LOOP
   * This should run as fast as possible (>1000 Hz) to maintain smooth stepping.
   */

  // 1. Read serial for new Position commands from Raspberry Pi
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    target_angle = cmd.toFloat();
    Serial.print("New target angle: ");
    Serial.println(target_angle);
  }
  
  // 2. Read true position from the magnetic encoder
  // AS5600 returns 0-4095. This scales it to 0-360 degrees.
  current_angle = encoder1.readAngle() * (360.0 / 4096.0);
  
  // 3. Simple P-control effort calculation
  float error = target_angle - current_angle;
  float speed = error * kp;
  
  // Hard cap the speed
  if (speed > 2000.0) speed = 2000.0;
  if (speed < -2000.0) speed = -2000.0;
  
  // Set the required speed
  stepper1.setSpeed(speed);
  
  // 4. Output the STEP/DIR signals based on the active speed.
  // This must be called constantly!
  stepper1.runSpeed();
}
