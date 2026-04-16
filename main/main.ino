/**
 * ESP32 Airmouse
 * Necessary Hardware: ESP32-C3, BMI160, 4 buttons
 */

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <Wire.h>
#include <BMI160Gen.h>  // Замість MPU6050.h

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
// "MPU6050 mpu" більше не потрібен, BMI160Gen використовується як синглтон через BMI160
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
  Wire.begin(8, 9);  // SDA=8, SCL=9 — ті самі піни, що були для MPU6050
  Serial.println("Ініціалізація BMI160...");

  // Адреса I2C: 0x68 (SA0→GND) або 0x69 (SA0→VCC)
  if (!BMI160.begin(BMI160GenClass::I2C_MODE, 0x68)) {
    Serial.println("Помилка: BMI160 не знайдено!");
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }
  // Аналог mpu.setDLPFMode(3) — встановлюємо частоту 100 Гц (~44 Гц смуга фільтра)
  BMI160.setGyroRate(BMI160_GYRO_RATE_100HZ);
  BMI160.setAccelerometerRate(BMI160_ACCEL_RATE_100HZ);

  // Діапазони (за замовчуванням: ±250°/с для гіро, ±2g для акселя)
  BMI160.setGyroRange(250);
  BMI160.setAccelerometerRange(2);

  delay(100);
  calibrateGyro();

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
  int16_t ax, ay, az, gx, gy, gz;
  BMI160.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  gx -= gyro.offsetX;
  gy -= gyro.offsetY;
  gz -= gyro.offsetZ;

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