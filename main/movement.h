/**
 * ESP32 Airmouse - Movement and Scroll Handlers
 */

#ifndef MOVEMENT_H
#define MOVEMENT_H

#include <Arduino.h>
#include "config.h"
#include "mouseFunctions.h"

// ============================================================================
// EXTERNAL VARIABLES
// ============================================================================
extern Config cfg;

// ============================================================================
// BTN_MOVE ISR
// ============================================================================

volatile bool moveState = false;
static volatile unsigned long lastMoveInterruptTime = 0;

void IRAM_ATTR moveISR() {
  unsigned long now = millis();
  if (now - lastMoveInterruptTime < DEBOUNCE_DELAY) return;
  lastMoveInterruptTime = now;
  moveState = (digitalRead(BTN_MOVE) == HIGH);  // TTP223: активний рівень HIGH
}

// ============================================================================
// MOVEMENT HANDLER
// ============================================================================

bool handleMovement(float gx, float gy, float gz, float dt) {
  // Convertation to degrees per second
  float gx_dps = -gz;
  float gy_dps = gx;
  
  // Dynamic threshold and sensitivity
  float avgRate = (abs(gx_dps) + abs(gy_dps)) / 2.0;
  float dynamicThreshold = cfg.baseThreshold + constrain(avgRate / 50.0, 0, 4);
  float speedFactor = constrain(avgRate / 50.0, 0.5, 4.0);
  
  float sensX = cfg.baseSensitivityX * speedFactor;
  float sensY = cfg.baseSensitivityY * speedFactor;
  
  // Delta calculation
  int dx = 0, dy = 0;
  if (abs(gx_dps) > dynamicThreshold) {
    dx = (int)(gx_dps * dt * sensX);
  }
  if (abs(gy_dps) > dynamicThreshold) {
    dy = (int)(gy_dps * dt * sensY);
  }
  
  // Result of calculations is dispatched to move
  if (dx != 0 || dy != 0) {
    dx = constrain(dx, -127, 127);
    dy = constrain(dy, -127, 127);
    mouseMove((signed char)dx, (signed char)dy);
    return true;
  }
  
  return false;
}

// ============================================================================
// SCROLL HANDLER
// ============================================================================

void handleScroll(float gx, float gy, float dt) {
  // Вертикальна вісь (нахил вперед/назад) — та сама, що й раніше
  float gy_dps  = gx;
  // Горизонтальна вісь (поворот навколо вертикалі) — дзеркально, як у handleMovement
  float gx_dps  = -gy;

  // !!! Vertical Scroll
  static float filteredRateV = 0;
  filteredRateV = cfg.alpha * filteredRateV + (1 - cfg.alpha) * abs(gy_dps);

  static float scrollAccumulatorV = 0;
  const float thresholdV = 12.0;

  if (filteredRateV >= thresholdV) {
    float baseSpeedV = abs(gy_dps) / 60.0;
    float speedFactorV = constrain(pow(baseSpeedV, 1.4), 0.2, 4.5);
    float scrollV = gy_dps * cfg.scrollSensitivityV * speedFactorV * dt;
    scrollAccumulatorV += scrollV;
  } else {
    // Плавне згасання акумулятора при слабкому русі
    scrollAccumulatorV *= 0.8;
  }

  int scrollY = (int)scrollAccumulatorV;
  if (scrollY != 0) {
    scrollY = constrain(scrollY, -127, 127);
    scrollAccumulatorV -= scrollY;
  }

  // !!! Horizontal Scroll
  static float filteredRateH = 0;
  filteredRateH = cfg.alpha * filteredRateH + (1 - cfg.alpha) * abs(gx_dps);

  static float scrollAccumulatorH = 0;
  const float thresholdH = 12.0;

  if (filteredRateH >= thresholdH) {
    float baseSpeedH = abs(gx_dps) / 60.0;
    float speedFactorH = constrain(pow(baseSpeedH, 1.4), 0.2, 4.5);
    float scrollH = gx_dps * cfg.scrollSensitivityH * speedFactorH * dt;
    scrollAccumulatorH += scrollH;
  } else {
    scrollAccumulatorH *= 0.8;
  }

  int scrollX = (int)scrollAccumulatorH;
  if (scrollX != 0) {
    scrollX = constrain(scrollX, -127, 127);
    scrollAccumulatorH -= scrollX;
  }

  // Delivery HID packet
  if (scrollY != 0 || scrollX != 0) {
    mouseScrollXY((signed char)scrollY, (signed char)scrollX);
  }
}

#endif