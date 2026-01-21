/**
 * ESP32 Airmouse - Mouse Functions
 */

#ifndef MOUSE_FUNCTIONS_H
#define MOUSE_FUNCTIONS_H

#include <Arduino.h>
#include "config.h"

// ============================================================================
// EXTERNAL VARIABLES
// ============================================================================
extern bool connected;
extern NimBLECharacteristic* inputMouse;
extern uint8_t currentButtons;

// ============================================================================
// MOUSE FUNCTIONS
// ============================================================================

void sendMouse(uint8_t buttons, signed char x, signed char y, signed char wheel, signed char hWheel) {
  if (!connected) return;
  
  uint8_t data[5] = { buttons, x, y, wheel, hWheel };
  inputMouse->setValue(data, 5);
  inputMouse->notify();
}

void mouseMove(signed char x, signed char y) {
  sendMouse(currentButtons, x, y, 0, 0);
}

void mouseClick(uint8_t button) {
  currentButtons = button;
  sendMouse(button, 0, 0, 0, 0);
  delay(50);
  currentButtons = 0;
  sendMouse(0, 0, 0, 0, 0);
}

void mousePress(uint8_t button) {
  currentButtons |= button;
  sendMouse(currentButtons, 0, 0, 0, 0);
}

void mouseRelease() {
  currentButtons = 0;
  sendMouse(0, 0, 0, 0, 0);
}

void mouseScroll(signed char wheel) {
  sendMouse(currentButtons, 0, 0, wheel, 0);
}

#endif