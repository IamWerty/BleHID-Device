/**
 * ESP32 Airmouse - Calibration Functions
 */

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>
#include "BMI160.h"
#include "config.h"

// ============================================================================
// EXTERNAL VARIABLES
// ============================================================================
extern BMI160 BMI;
extern Offset off;
extern unsigned long lastCalibrationTime;
extern Buttons btn;

// ============================================================================
// CALIBRATION FUNCTIONS
// ============================================================================

void calibrateGyro() {
  Serial.println("Калібрування... Тримайте нерухомо!");
  off = BMI.calibrate();  // calibrate() вже усереднює 500 семплів і повертає Offset
  Serial.println("Калібрування завершено!");
  Serial.printf("Офсети: gx=%.4f, gy=%.4f, gz=%.4f\n", off.gx, off.gy, off.gz);
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