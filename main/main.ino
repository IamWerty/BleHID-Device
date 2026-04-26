/**
 * ESP32 Airmouse
 * Necessary Hardware: ESP32-C3, BMI160, 4 buttons
 */

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <Wire.h>
#include <BMI160.h>

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
BMI160 BMI;
Offset off;
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
// BMI160 INITIALIZATION
// ============================================================================

void initBMI160() {
  Wire.begin(8, 9);  // SDA=8, SCL=9
  Serial.println("Ініціалізація BMI160...");
  BMI.init(4, 500);       // +-4g, +-500 degrees per second
  BMI.connectionTest();
  calibrateGyro();        // off заповнюється всередині calibrateGyro()
  Serial.println("BMI160 готово!");
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
  initBMI160();  // Замість initMPU6050()

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
  btn.move  = (digitalRead(BTN_MOVE)   == HIGH);
  btn.left  = (digitalRead(BTN_LCLICK) == LOW);
  btn.right = (digitalRead(BTN_RCLICK) == LOW);

  // Control button handle
  handleControlButton();

  // Calibrations
  checkPeriodicCalibration();
  handleAutoCalibration();

  // Читання даних з BMI160 (аналог mpu.getMotion6)
  SensorData d = BMI.readCalibrated(off);
  
  // Low-pass filter
  gyro.filteredX = cfg.alpha * gyro.filteredX + (1 - cfg.alpha) * d.gx;
  gyro.filteredY = cfg.alpha * gyro.filteredY + (1 - cfg.alpha) * d.gy;
  gyro.filteredZ = cfg.alpha * gyro.filteredZ + (1 - cfg.alpha) * d.gz;

  // MOVE/SCROLL handle
  if (mode == SCROLL && btn.move) {
    handleScroll(gyro.filteredX, gyro.filteredY, dt);
  } else if (btn.move) {
    bool moved = handleMovement(gyro.filteredX, gyro.filteredY, gyro.filteredZ, dt);
    if (moved) {
      lastMoveTime = millis();
    }
  }

  // Clicks handle
  handleClicks(btn.left, btn.right, btn.move);
} 