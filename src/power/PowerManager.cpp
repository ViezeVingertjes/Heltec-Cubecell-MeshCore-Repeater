#include "PowerManager.h"
#include "../core/Config.h"
#include "../core/Logger.h"

// CubeCell low-power API
extern "C" {
  // Light sleep - preserves RAM and peripherals, wakes on interrupt
  // This is the correct API for ASR6501 on CubeCell
  void lowPowerHandler(void);
}

PowerManager &PowerManager::getInstance() {
  static PowerManager instance;
  return instance;
}

void PowerManager::initialize() {
  LOG_INFO("Power management initialized");
  if (Config::Power::LIGHT_SLEEP_ENABLED) {
    LOG_INFO("Light sleep mode enabled");
  }
}

void PowerManager::sleep(uint32_t maxSleepMs) {
  if (!sleepEnabled || !Config::Power::LIGHT_SLEEP_ENABLED) {
    return;
  }

  if (!canSleep()) {
    return;
  }

  // Power optimization: Enter light sleep immediately
  // The lowPowerHandler() will wake on any interrupt (LoRa, etc.)
  uint32_t sleepStart = millis();
  
  lowPowerHandler();
  
  // Handle millis() overflow safely: the subtraction works correctly due to
  // unsigned integer wraparound, but only accumulate if the result is reasonable
  // (less than maxSleepMs or 10 seconds if maxSleepMs is 0)
  uint32_t sleepEnd = millis();
  uint32_t sleepDuration = sleepEnd - sleepStart;
  uint32_t maxReasonable = (maxSleepMs > 0) ? maxSleepMs + 1000 : 10000;
  
  // Only accumulate if duration is reasonable (protects against millis() overflow glitches)
  if (sleepDuration < maxReasonable) {
    totalSleepTimeMs += sleepDuration;
    sleepCycles++;
  } else {
    LOG_WARN_FMT("Ignoring suspicious sleep duration: %lu ms", sleepDuration);
  }

  LOG_DEBUG_FMT("Slept for %lu ms (total: %lu ms, cycles: %lu)", 
                sleepDuration, totalSleepTimeMs, sleepCycles);
}

bool PowerManager::canSleep() const { return true; }

void PowerManager::preventSleep() {
  sleepEnabled = false;
  LOG_DEBUG("Sleep disabled");
}

void PowerManager::allowSleep() {
  sleepEnabled = true;
  LOG_DEBUG("Sleep enabled");
}

