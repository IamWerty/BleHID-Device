/**
 * ESP32 Airmouse - Control Button Handler
 */

#ifndef CONTROL_BUTTON_H
#define CONTROL_BUTTON_H

#include <Arduino.h>
#include "config.h"
#include "calibration.h"

// ============================================================================
// EXTERNAL VARIABLES
// ============================================================================
extern ControlButton controlBtn;
extern Mode mode;
extern unsigned long lastCalibrationTime;

// ============================================================================
// INTERRUPT HANDLER
// ============================================================================

void IRAM_ATTR handleControlInterrupt() {
  unsigned long now = millis();
  if (now - controlBtn.lastInterruptTime < DEBOUNCE_DELAY) return;
  
  bool currentState = (digitalRead(BTN_CONTROL) == LOW);
  
  if (currentState && !controlBtn.isPressed) {
    controlBtn.pressStartTime = now;
    controlBtn.isPressed = true;
  }
  
  controlBtn.lastInterruptTime = now;
}

// ============================================================================
// BUTTON HANDLER
// ============================================================================

void handleControlButton() {
  unsigned long now = millis();
  bool buttonState = (digitalRead(BTN_CONTROL) == LOW);
  
  // Long press - calibration
  if (controlBtn.isPressed && buttonState) {
    unsigned long pressDuration = now - controlBtn.pressStartTime;
    
    if (pressDuration > LONG_PRESS_THRESHOLD && !controlBtn.wasLongPress) {
      Serial.println("Калібрування (довге утримання)");
      calibrateGyro();
      lastCalibrationTime = millis();
      controlBtn.wasLongPress = true;
    }
  }
  
  // Release button
  if (controlBtn.isPressed && !buttonState) {
    unsigned long pressDuration = now - controlBtn.pressStartTime;
    
    // Short press - switch mode
    if (pressDuration < LONG_PRESS_THRESHOLD && !controlBtn.wasLongPress) {
      mode = (mode == MOVE) ? SCROLL : MOVE;
      Serial.printf("Режим: %s\n", mode == SCROLL ? "Прокрутка" : "Рух");
    }
    
    controlBtn.isPressed = false;
    controlBtn.wasLongPress = false;
  }
}

#endif