/**
 * ESP32 Airmouse - Calibration Functions
 */

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>
#include <BMI160Gen.h>
#include "config.h"

// ============================================================================
// EXTERNAL VARIABLES
// ============================================================================
extern GyroData gyro;
extern Buttons btn;
extern unsigned long lastCalibrationTime;

// ============================================================================
// CALIBRATION FUNCTIONS
// ============================================================================

void calibrateGyro() {
  const int samples = 100;
  long sumX = 0, sumY = 0, sumZ = 0;
  
  Serial.println("Калібрування... Тримайте нерухомо!");
  
  for (int i = 0; i < samples; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    BMI160.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sumX += gx;
    sumY += gy;
    sumZ += gz;
    if (i == 0) Serial.println("Перше зчитування OK");
    delay(2);
  }
  
  gyro.offsetX = sumX / samples;
  gyro.offsetY = sumY / samples;
  gyro.offsetZ = sumZ / samples;
  
  Serial.println("Калібрування завершено!");
  Serial.printf("Офсети: gx=%ld, gy=%ld, gz=%ld\n", gyro.offsetX, gyro.offsetY, gyro.offsetZ);
}

void handleAutoCalibration() {
  static unsigned long lastInactive = 0;
  static bool movePressedPrev = false;

  if (!btn.move && !movePressedPrev && lastInactive == 0) {
    lastInactive = millis();
  }
  
  if (btn.move) {
    lastInactive = 0;
  }
  
  if (lastInactive > 0 && (millis() - lastInactive > AUTO_CALIBRATION_DELAY)) {
    Serial.println("Автокалібрування (60 с неактивності)...");
    calibrateGyro();
    lastCalibrationTime = millis();
    lastInactive = 0;
  }
  
  movePressedPrev = btn.move;
}

void checkPeriodicCalibration() {
  if (millis() - lastCalibrationTime >= CALIBRATION_INTERVAL) {
    Serial.println("Автоматична калібрування (10 хв)");
    calibrateGyro();
    lastCalibrationTime = millis();
  }
}

#endif