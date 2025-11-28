#pragma once

#include <Arduino.h>

/**
 * LED indicator for RX/TX activity using the onboard RGB LED.
 * Vext power rail is dynamically controlled to save power.
 */
class LEDIndicator {
public:
  static LEDIndicator &getInstance();

  void initialize();
  void flashRX();
  void flashTX();
  void loop();

private:
  LEDIndicator() : ledOn(false), vextOn(false), ledOffTime(0) {}

  bool ledOn;
  bool vextOn;
  uint32_t ledOffTime;

  void turnOff();
  void enableVext();
  void disableVext();

  LEDIndicator(const LEDIndicator &) = delete;
  LEDIndicator &operator=(const LEDIndicator &) = delete;
};

