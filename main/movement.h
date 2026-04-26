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
  float gy_dps = gx;
  
  // Filtration
  static float filteredRate = 0;
  filteredRate = cfg.alpha * filteredRate + (1 - cfg.alpha) * abs(gy_dps);
  
  float threshold = 12.0;
  if (filteredRate < threshold) return;
  
  // Calculation on scroll speed
  float baseSpeed = abs(gy_dps) / 60.0;
  float scrollSpeedFactor = pow(baseSpeed, 1.4);
  scrollSpeedFactor = constrain(scrollSpeedFactor, 0.2, 4.5);
  
  float scroll = gy_dps * cfg.scrollSensitivity * scrollSpeedFactor * dt;
  
  // Accumulation and dispatch results
  static float scrollAccumulator = 0;
  scrollAccumulator += scroll;
  int scrollY = (int)scrollAccumulator;
  
  if (scrollY != 0) {
    scrollY = constrain(scrollY, -127, 127);
    mouseScroll((signed char)scrollY);
    scrollAccumulator -= scrollY;
  }
}

#endif