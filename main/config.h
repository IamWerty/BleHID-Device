/**
 * ESP32 Airmouse - Configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// PINS
// ============================================================================
#define LED_PIN 8
#define BTN_MOVE 5
#define BTN_LCLICK 4
#define BTN_RCLICK 2
#define BTN_CONTROL 3

// ============================================================================
// TIME INTERVALS
// ============================================================================
#define CALIBRATION_INTERVAL 600000  // 10 minutes
#define AUTO_CALIBRATION_DELAY 60000 // 60 seconds/1 minute inactivity
#define DEBOUNCE_DELAY 50            // milliseconds
#define LONG_PRESS_THRESHOLD 1000    // milliseconds
#define CLICK_HOLD_THRESHOLD 150     // milliseconds

// ============================================================================
// DATA STRUCTURES
// ============================================================================

enum Mode { 
  MOVE, 
  SCROLL 
};

struct Buttons {
  bool move;
  bool left;
  bool right;
};

struct Config {
  float baseSensitivityX = 18.0;
  float baseSensitivityY = 22.0;
  float baseThreshold = 2.0;
  float scrollSensitivity = 0.5;
  int maxScroll = 3;
  float alpha = 0.8;
  unsigned long clickDelayAfterMove = 200;
};

struct GyroData {
  long offsetX = 0;
  long offsetY = 0;
  long offsetZ = 0;
  float filteredX = 0;
  float filteredY = 0;
  float filteredZ = 0;
};

struct ControlButton {
  volatile bool pressed = false;
  unsigned long pressStartTime = 0;
  bool isPressed = false;
  bool wasLongPress = false;
  unsigned long lastInterruptTime = 0;
};

#endif