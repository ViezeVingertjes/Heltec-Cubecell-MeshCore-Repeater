#pragma once

#include <Arduino.h>

/**
 * LED indicator for RX/TX activity using the onboard RGB LED
 * Simple, non-blocking flashes to avoid interfering with radio operations
 */
class LEDIndicator {
public:
  static LEDIndicator &getInstance();

  void initialize();
  void flashRX();     // Green flash for packet received
  void flashTX();     // Blue flash for packet transmitted
  void loop();        // Call in main loop to handle LED timing

private:
  LEDIndicator() : ledOn(false), ledOffTime(0) {}

  bool ledOn;
  uint32_t ledOffTime;

  void turnOff();

  LEDIndicator(const LEDIndicator &) = delete;
  LEDIndicator &operator=(const LEDIndicator &) = delete;
};

