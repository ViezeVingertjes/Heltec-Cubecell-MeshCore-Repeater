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

  uint32_t sleepStart = millis();
  lowPowerHandler();
  
  uint32_t sleepDuration = millis() - sleepStart;
  totalSleepTimeMs += sleepDuration;
  sleepCycles++;

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

