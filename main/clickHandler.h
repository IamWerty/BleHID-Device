/**
 * ESP32 Airmouse - Click Handler
 * 
 * Зміни: додано ISR для BTN_LCLICK і BTN_RCLICK.
 * Підключення: викликати attachClickInterrupts() у setup() після pinMode.
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
// ISR STATE  (volatile — читається і з ISR, і з loop)
// ============================================================================

// Зберігаємо стан піна прямо в ISR, щоб не робити digitalRead у loop
volatile bool lClickState = false;  // true = зараз натиснута
volatile bool rClickState = false;

// ============================================================================
// ISR — спрацьовують на БУДЬ-ЯКУ зміну (CHANGE):
//   натискання (HIGH→LOW при pullup) і відпускання (LOW→HIGH)
// ============================================================================

void IRAM_ATTR lClickISR() {
  // digitalRead всередині ISR — допустимо, швидко (~100 нс)
  lClickState = (digitalRead(BTN_LCLICK) == LOW);
}

void IRAM_ATTR rClickISR() {
  rClickState = (digitalRead(BTN_RCLICK) == LOW);
}

// Викликати один раз у setup(), після pinMode для BTN_LCLICK / BTN_RCLICK
void attachClickInterrupts() {
  attachInterrupt(BTN_LCLICK, lClickISR, CHANGE);
  attachInterrupt(BTN_RCLICK, rClickISR, CHANGE);
}

// ============================================================================
// CLICK HANDLER
// ============================================================================

void handleClicks(bool movePressed) {
  // Читаємо volatile один раз на початку, щоб стан не змінився посередині функції
  bool lClick = lClickState;
  bool rClick = rClickState;

  static unsigned long leftPressTime  = 0;
  static unsigned long rightPressTime = 0;
  static bool leftWasPressed  = false;
  static bool rightWasPressed = false;

  unsigned long now       = millis();
  unsigned long sinceMove = now - lastMoveTime;

  // Ліва кнопка миші
  if (lClick) {
    if (!leftWasPressed) {
      leftPressTime   = now;
      leftWasPressed  = true;
    }

    // Довге утримання - drag
    if (!leftHeld && (now - leftPressTime > CLICK_HOLD_THRESHOLD)) {
      mousePress(0x01);
      leftHeld = true;
      Serial.println("ЛКМ зажата");
    }
  } else {
    if (leftHeld) {
      mouseRelease();
      leftHeld = false;
      Serial.println("ЛКМ відпущена");
    } else if (leftWasPressed
               && (now - leftPressTime <= CLICK_HOLD_THRESHOLD)
               && sinceMove > cfg.clickDelayAfterMove) {
      mouseClick(0x01);
      Serial.println("ЛКМ клік");
      delay(150);
    }
    leftWasPressed = false;
  }

  // Права кнопка миші
  if (rClick) {
    if (!rightWasPressed) {
      rightPressTime  = now;
      rightWasPressed = true;
    }

    if (!rightHeld && (now - rightPressTime > CLICK_HOLD_THRESHOLD)) {
      mousePress(0x02);
      rightHeld = true;
      Serial.println("ПКМ зажата");
    }
  } else {
    if (rightHeld) {
      mouseRelease();
      rightHeld = false;
      Serial.println("ПКМ відпущена");
    } else if (rightWasPressed
               && (now - rightPressTime <= CLICK_HOLD_THRESHOLD)
               && sinceMove > cfg.clickDelayAfterMove) {
      mouseClick(0x02);
      Serial.println("ПКМ клік");
      delay(150);
    }
    rightWasPressed = false;
  }
}

#endif