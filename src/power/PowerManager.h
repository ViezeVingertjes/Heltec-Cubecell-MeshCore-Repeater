#pragma once

#include <Arduino.h>

/**
 * Power Management for Heltec CubeCell (ASR6501)
 * 
 * Provides light sleep mode while maintaining continuous radio reception.
 * The MCU sleeps between operations and wakes on radio interrupts.
 */
class PowerManager {
public:
  static PowerManager &getInstance();

  void initialize();
  void sleep(uint32_t maxSleepMs = 0);
  bool canSleep() const;
  void preventSleep();
  void allowSleep();
  
  uint32_t getTotalSleepTime() const { return totalSleepTimeMs; }
  uint32_t getSleepCycles() const { return sleepCycles; }

private:
  PowerManager() : sleepEnabled(true), totalSleepTimeMs(0), sleepCycles(0) {}

  bool sleepEnabled;
  uint32_t totalSleepTimeMs;
  uint32_t sleepCycles;

  PowerManager(const PowerManager &) = delete;
  PowerManager &operator=(const PowerManager &) = delete;
};

