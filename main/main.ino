/**
 * ESP32 Airmouse
 * Necesary Hardware: ESP32-C3, MPU6050, 4 buttons
 */

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <HIDTypes.h>
#include <Wire.h>
#include "MPU6050.h"

// ============================================================================
// CONSTS AND SETTINGS
// ============================================================================

// Піни
#define LED_PIN 8
#define BTN_MOVE 5
#define BTN_LCLICK 4
#define BTN_RCLICK 2
#define BTN_CONTROL 3

// Інтервали часу:
#define CALIBRATION_INTERVAL 600000  // 10 minutes
#define AUTO_CALIBRATION_DELAY 60000 // 60 seconds/1 minute unactivity
#define DEBOUNCE_DELAY 50            // miliseconds
#define LONG_PRESS_THRESHOLD 1000    // miliseconds
#define CLICK_HOLD_THRESHOLD 150     // miliseconds

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
};

struct ControlButton {
  volatile bool pressed = false;
  unsigned long pressStartTime = 0;
  bool isPressed = false;
  bool wasLongPress = false;
  unsigned long lastInterruptTime = 0;
};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// BLE
bool connected = false;
NimBLEHIDDevice* hid;
NimBLECharacteristic* inputMouse;

// SENSORS
MPU6050 mpu;
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
// HID DESCRIPTOR
// ============================================================================

static const uint8_t _hidReportDescriptor[] = {
  USAGE_PAGE(1), 0x01,       // Generic Desktop
  USAGE(1), 0x02,            // Mouse
  COLLECTION(1), 0x01,       // Application
  USAGE(1), 0x01,            // Pointer
  COLLECTION(1), 0x00,       // Physical
  
  // BUTTONS (Left, Right, Middle, Back, Forward)
  USAGE_PAGE(1), 0x09,
  USAGE_MINIMUM(1), 0x01,
  USAGE_MAXIMUM(1), 0x05,
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,
  REPORT_SIZE(1), 0x01,
  REPORT_COUNT(1), 0x05,
  HIDINPUT(1), 0x02,
  
  // Padding
  REPORT_SIZE(1), 0x03,
  REPORT_COUNT(1), 0x01,
  HIDINPUT(1), 0x03,
  
  // X/Y position, Wheel
  USAGE_PAGE(1), 0x01,
  USAGE(1), 0x30,            // X
  USAGE(1), 0x31,            // Y
  USAGE(1), 0x38,            // Wheel
  LOGICAL_MINIMUM(1), 0x81,
  LOGICAL_MAXIMUM(1), 0x7f,
  REPORT_SIZE(1), 0x08,
  REPORT_COUNT(1), 0x03,
  HIDINPUT(1), 0x06,
  
  // Horizontal wheel
  USAGE_PAGE(1), 0x0c,
  USAGE(2), 0x38, 0x02,
  LOGICAL_MINIMUM(1), 0x81,
  LOGICAL_MAXIMUM(1), 0x7f,
  REPORT_SIZE(1), 0x08,
  REPORT_COUNT(1), 0x01,
  HIDINPUT(1), 0x06,
  
  END_COLLECTION(0),
  END_COLLECTION(0)
};

// ============================================================================
// BLE CALLBACKS
// ============================================================================

class BLECallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    connected = true;
    digitalWrite(LED_PIN, HIGH);
    Serial.println("BLE з'єднано");
  }
  
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    connected = false;
    digitalWrite(LED_PIN, LOW);
    Serial.println("BLE роз'єднано");
    pServer->startAdvertising();
  }
};

// ============================================================================
// MOUSE FUNCTIONS
// ============================================================================

void sendMouse(uint8_t buttons, signed char x, signed char y, 
               signed char wheel, signed char hWheel) {
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

// ============================================================================
// CALIBRATIONS
// ============================================================================

void calibrateGyro() {
  const int samples = 1000;
  long sumX = 0, sumY = 0, sumZ = 0;
  
  Serial.println("Калібрування... Тримайте нерухомо!");
  
  for (int i = 0; i < samples; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sumX += gx;
    sumY += gy;
    sumZ += gz;
    delay(2);
  }
  
  gyro.offsetX = sumX / samples;
  gyro.offsetY = sumY / samples;
  gyro.offsetZ = sumZ / samples;
  
  Serial.println("Калібрування завершено!");
  Serial.printf("Офсети: gx=%ld, gy=%ld, gz=%ld\n", 
                gyro.offsetX, gyro.offsetY, gyro.offsetZ);
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

// ============================================================================
// CONTROL BUTTON HANDLE
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

void handleControlButton() {
  unsigned long now = millis();
  bool buttonState = (digitalRead(BTN_CONTROL) == LOW);
  
  // long press - calibration
  if (controlBtn.isPressed && buttonState) {
    unsigned long pressDuration = now - controlBtn.pressStartTime;
    
    if (pressDuration > LONG_PRESS_THRESHOLD && !controlBtn.wasLongPress) {
      Serial.println("Калібрування (довге утримання)");
      calibrateGyro();
      lastCalibrationTime = millis();
      controlBtn.wasLongPress = true;
    }
  }
  
  // release button
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

// ============================================================================
// MOVE HANDLE
// ============================================================================

bool handleMovement(float gx, float gy, float gz, float dt) {
  // Конвертація в градуси за секунду
  float gx_dps = -gz / 131.0;
  float gy_dps = -gx / 131.0;
  
  // Dynamic threshold and sensitivity
  float avgRate = (abs(gx_dps) + abs(gy_dps)) / 2.0;
  float dynamicThreshold = cfg.baseThreshold + constrain(avgRate / 50.0, 0, 4);
  float speedFactor = constrain(avgRate / 50.0, 0.5, 4.0);
  
  float sensX = cfg.baseSensitivityX * speedFactor;
  float sensY = cfg.baseSensitivityY * speedFactor;
  
  // delta calculation
  int dx = 0, dy = 0;
  if (abs(gx_dps) > dynamicThreshold) {
    dx = (int)(gx_dps * dt * sensX);
  }
  if (abs(gy_dps) > dynamicThreshold) {
    dy = (int)(gy_dps * dt * sensY);
  }
  
  // result of calculations is dispatch to move
  if (dx != 0 || dy != 0) {
    dx = constrain(dx, -127, 127);
    dy = constrain(dy, -127, 127);
    mouseMove((signed char)dx, (signed char)dy);
    return true;
  }
  
  return false;
}

// ============================================================================
// SCROLL HANDLE
// ============================================================================

void handleScroll(float gx, float gy, float dt) {
  float gy_dps = -gx / 131.0;
  
  // Filtration
  static float filteredRate = 0;
  filteredRate = cfg.alpha * filteredRate + (1 - cfg.alpha) * abs(gy_dps);
  
  float threshold = 12.0;
  if (filteredRate < threshold) return;
  
  // calculation on scroll speed
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

// ============================================================================
// CLICKS HANDLE
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
    
    // long press - drag
    if (leftWasPressed && !leftHeld && 
        (now - leftPressTime > CLICK_HOLD_THRESHOLD)) {
      mousePress(0x01);
      leftHeld = true;
      Serial.println("ЛКМ зажата");
    }
  } else {
    if (leftHeld) {
      mouseRelease();
      leftHeld = false;
      Serial.println("ЛКМ відпущена");
    } else if (leftWasPressed && 
               (now - leftPressTime <= CLICK_HOLD_THRESHOLD) &&
               sinceMove > cfg.clickDelayAfterMove) {
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
    
    // long press
    if (rightWasPressed && !rightHeld && 
        (now - rightPressTime > CLICK_HOLD_THRESHOLD)) {
      mousePress(0x02);
      rightHeld = true;
      Serial.println("ПКМ зажата");
    }
  } else {
    if (rightHeld) {
      mouseRelease();
      rightHeld = false;
      Serial.println("ПКМ відпущена");
    } else if (rightWasPressed && 
               (now - rightPressTime <= CLICK_HOLD_THRESHOLD) &&
               sinceMove > cfg.clickDelayAfterMove) {
      mouseClick(0x02);
      Serial.println("ПКМ клік");
      delay(150);
    }
    rightWasPressed = false;
  }
}

// ============================================================================
// INITIALIZATION BLE
// ============================================================================

void initBLE() {
  Serial.println("Ініціалізація BLE...");
  
  NimBLEDevice::init("Airmouse");
  NimBLEDevice::setSecurityAuth(true, true, true);
  
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new BLECallbacks());
  
  hid = new NimBLEHIDDevice(pServer);
  inputMouse = hid->getInputReport(0);
  hid->setManufacturer("protomors");
  hid->setPnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->setHidInfo(0x00, 0x02);
  hid->setReportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  hid->startServices();
  
  NimBLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_MOUSE);
  pAdvertising->addServiceUUID(hid->getHidService()->getUUID());
  
  NimBLEAdvertisementData scanRespData;
  scanRespData.setName("Airmouse");
  pAdvertising->setScanResponseData(scanRespData);
  
  pAdvertising->setMinInterval(0x20);
  pAdvertising->setMaxInterval(0x40);
  pAdvertising->start();
  
  hid->setBatteryLevel(80);
  
  Serial.println("BLE готово!");
}

// ============================================================================
// INITIALIZATION MPU6050
// ============================================================================

void initMPU6050() {
  Wire.begin(8, 9);
  Serial.println("Ініціалізація MPU6050...");
  
  mpu.initialize();
  
  if (!mpu.testConnection()) {
    Serial.println("Помилка: MPU6050 не знайдено!");
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }

  mpu.setDLPFMode(3);
  calibrateGyro();
  
  Serial.println("MPU6050 готово!");
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
  
  attachInterrupt(digitalPinToInterrupt(BTN_CONTROL), 
                  handleControlInterrupt, FALLING);
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  setCpuFrequencyMhz(80);
  
  initPins();
  initBLE();
  initMPU6050();
  
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

  // timers
  unsigned long now = micros();
  float dt = (now - lastMicros) / 1e6;
  if (dt < 0.01) return;
  lastMicros = now;

  // update buttons state
  btn.move = (digitalRead(BTN_MOVE) == HIGH);
  btn.left = (digitalRead(BTN_LCLICK) == LOW);
  btn.right = (digitalRead(BTN_RCLICK) == LOW);

  // ctrl button handle
  handleControlButton();

  // calibrations
  checkPeriodicCalibration();
  handleAutoCalibration();

  // data read from MPU6050
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // subtraction offset
  gx -= gyro.offsetX;
  gy -= gyro.offsetY;
  gz -= gyro.offsetZ;

  // Low-pass filter
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

  // clicks handle
  handleClicks(btn.left, btn.right, btn.move);
}
