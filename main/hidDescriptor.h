/**
 * ESP32 Airmouse - HID Descriptor
 */

#ifndef HID_DESCRIPTOR_H
#define HID_DESCRIPTOR_H

#include <HIDTypes.h>

// ============================================================================
// HID REPORT DESCRIPTOR
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

#endif