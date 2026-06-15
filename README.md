# BleHID-Device
Repo for project to ESP32-C3 BleHID Device, that works like mouse.
## Libraries
In project uses next libraries:
- Wire(for I2C)
- NimBLE by h2zero
- BMI160 by IamWerty
## Functions
 - BLE connection
 - Right Mouse Click
 - Left Mouse Click
 - Move/Scrool modes
 - Gyro filtration
## Q&A
 > I have: "error: 'BMI160' does not name a type BMI160 {Name};", what does it means?
 ---
 If you have another BMI160 Library, try to remove them, and compilate sketch again.