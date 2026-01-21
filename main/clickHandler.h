/**
 * ESP32 Airmouse - Click Handler
 */

#ifndef CLICK_HANDLER_H
#define CLICK_HANDLER_H

#include <Arduino.h>
#include "config.h"
#include "mouseFunctions.h"

// ============================================================================
// EXTERNAL VARIABLES
// ============================================================================
extern Config cfg;
extern unsigned long lastMoveTime;
extern bool leftHeld;
extern bool rightHeld;

// ============================================================================
// CLICK HANDLER
// ============================================================================

void handleClicks(bool lClick, bool rClick, bool movePressed) {
  static unsigned long leftPressTime = 0;
  static unsigned long rightPressTime = 0;
  static bool leftWasPressed = false;
  static bool rightWasPressed = false;
  
  unsigned long now = millis();
  unsigned long sinceMove = now - lastMoveTime;

  // Left Mouse Button
  if (lClick) {
    if (!leftWasPressed) {
      leftPressTime = now;
      leftWasPressed = true;
    }
    
    // Long press - drag
    if (leftWasPressed && !leftHeld && (now - leftPressTime > CLICK_HOLD_THRESHOLD)) {
      mousePress(0x01);
      leftHeld = true;
      Serial.println("ЛКМ зажата");
    }
  } else {
    if (leftHeld) {
      mouseRelease();
      leftHeld = false;
      Serial.println("ЛКМ відпущена");
    } else if (leftWasPressed && (now - leftPressTime <= CLICK_HOLD_THRESHOLD) && sinceMove > cfg.clickDelayAfterMove) {
      mouseClick(0x01);
      Serial.println("ЛКМ клік");
      delay(150);
    }
    leftWasPressed = false;
  }

  // Right Mouse Button
  if (rClick) {
    if (!rightWasPressed) {
      rightPressTime = now;
      rightWasPressed = true;
    }
    
    // Long press
    if (rightWasPressed && !rightHeld && (now - rightPressTime > CLICK_HOLD_THRESHOLD)) {
      mousePress(0x02);
      rightHeld = true;
      Serial.println("ПКМ зажата");
    }
  } else {
    if (rightHeld) {
      mouseRelease();
      rightHeld = false;
      Serial.println("ПКМ відпущена");
    } else if (rightWasPressed && (now - rightPressTime <= CLICK_HOLD_THRESHOLD) && sinceMove > cfg.clickDelayAfterMove) {
      mouseClick(0x02);
      Serial.println("ПКМ клік");
      delay(150);
    }
    rightWasPressed = false;
  }
}

#endif