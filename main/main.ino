/**
 * ESP32 Airmouse
 * Necessary Hardware: ESP32-C3, MPU6050, 4 buttons
 */

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <Wire.h>
#include "MPU6050.h"

// Project headers
#include "config.h"
#include "hidDescriptor.h"
#include "bleHandler.h"
#include "mouseFunctions.h"
#include "calibration.h"
#include "controlButton.h"
#include "movement.h"
#include "clickHandler.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// BLE
bool connected = false;
NimBLEHIDDevice* hid;
NimBLECharacteristic* inputMouse;

// SENSORS
MPU6050 mpu;
GyroData gyro;

// STATUSES
Buttons btn;
Mode mode = MOVE;
Config cfg;
ControlButton controlBtn;

// TIMERS
unsigned long lastMicros = 0;
unsigned long lastMoveTime = 0;
unsigned long lastCalibrationTime = 0;

// STATUS OF MOUSE BUTTONS
bool leftHeld = false;
bool rightHeld = false;
uint8_t currentButtons = 0;

// ============================================================================
// MPU6050 INITIALIZATION
// ============================================================================

void initMPU6050() {
  Wire.begin(8, 9);
  Serial.println("Ініціалізація MPU6050...");
  
  mpu.initialize();
  
  if (!mpu.testConnection()) {
    Serial.println("Помилка: MPU6050 не знайдено!");
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }

  mpu.setDLPFMode(3);
  calibrateGyro();
  
  Serial.println("MPU6050 готово!");
}

// ============================================================================
// PINS INITIALIZATION
// ============================================================================

void initPins() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_MOVE, INPUT_PULLUP);
  pinMode(BTN_RCLICK, INPUT_PULLUP);
  pinMode(BTN_LCLICK, INPUT_PULLUP);
  pinMode(BTN_CONTROL, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(BTN_CONTROL), handleControlInterrupt, FALLING);
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  setCpuFrequencyMhz(80);
  
  initPins();
  initBLE();
  initMPU6050();
  
  lastMicros = micros();
  lastCalibrationTime = millis();

  Serial.println("Готово!");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  if (!connected) {
    delay(100);
    return;
  }

  // Timers
  unsigned long now = micros();
  float dt = (now - lastMicros) / 1e6;
  if (dt < 0.01) return;
  lastMicros = now;

  // Update buttons state
  btn.move = (digitalRead(BTN_MOVE) == HIGH);
  btn.left = (digitalRead(BTN_LCLICK) == LOW);
  btn.right = (digitalRead(BTN_RCLICK) == LOW);

  // Control button handle
  handleControlButton();

  // Calibrations
  checkPeriodicCalibration();
  handleAutoCalibration();

  // Data read from MPU6050
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // Subtraction offset
  gx -= gyro.offsetX;
  gy -= gyro.offsetY;
  gz -= gyro.offsetZ;

  // Low-pass filter
  gyro.filteredX = cfg.alpha * gyro.filteredX + (1 - cfg.alpha) * gx;
  gyro.filteredY = cfg.alpha * gyro.filteredY + (1 - cfg.alpha) * gy;

  // MOVE/SCROLL handle
  if (mode == SCROLL && btn.move) {
    handleScroll(gyro.filteredX, gyro.filteredY, dt);
  } else if (btn.move) {
    bool moved = handleMovement(gyro.filteredX, gyro.filteredY, gz, dt);
    if (moved) {
      lastMoveTime = millis();
    }
  }

  // Clicks handle
  handleClicks(btn.left, btn.right, btn.move);
}