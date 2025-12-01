#pragma once

#include <Arduino.h>

class BatteryMonitor {
public:
  BatteryMonitor()
      : lastCheckMillis(0), projectionStageMask(0), lowBatterySent(false) {}

  void initialize() {}
  void loop();

private:
  static constexpr uint32_t CHECK_INTERVAL_MS = 5 * 60 * 1000UL; // 5 minutes
  static constexpr uint8_t LOW_PERCENT_THRESHOLD = 15;           // ~1 day left
  static constexpr uint8_t RESET_THRESHOLD = LOW_PERCENT_THRESHOLD + 5;
  static const uint8_t PROJECTION_STEPS[6];
  static constexpr uint8_t PROJECTION_STEP_COUNT = 6;

  uint32_t lastCheckMillis;
  uint8_t projectionStageMask;
  bool lowBatterySent;

  void sendDistress(uint16_t batteryMv, uint8_t percent, int32_t hoursRemaining);
  void resetProjection();
  bool shouldTriggerThreshold(int32_t hoursRemaining, uint8_t &triggeredStage);
};

