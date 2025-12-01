#include "BatteryMonitor.h"
#include "BatteryUtils.h"
#include "../mesh/PublicChannelAnnouncer.h"
#include "../core/NodeConfig.h"
#include "../core/TimeSync.h"
#include "BatteryEstimator.h"

const uint8_t BatteryMonitor::PROJECTION_STEPS[6] = {72, 48, 24, 12, 6, 3};

void BatteryMonitor::loop() {
  uint32_t now = millis();
  if (now - lastCheckMillis < CHECK_INTERVAL_MS) {
    return;
  }
  lastCheckMillis = now;

  uint16_t batteryMv = getBatteryVoltage();
  if (BatteryUtils::isUsbPowered(batteryMv)) {
    BatteryEstimator::reset();
    resetProjection();
    return;
  }

  uint8_t percent = BatteryUtils::estimatePercent(batteryMv);
  if (percent >= RESET_THRESHOLD) {
    resetProjection();
  }

  uint32_t timestamp = millis() / 1000;
  BatteryEstimator::addSample(timestamp, percent);

  int32_t hoursRemaining = -1;
  bool hasProjection = BatteryEstimator::estimateHours(hoursRemaining);
  uint8_t stageHit = 0;
  bool trigger = hasProjection && shouldTriggerThreshold(hoursRemaining, stageHit);
  
  // Send distress message for low battery (once) or projection threshold triggers
  if (percent <= LOW_PERCENT_THRESHOLD && !lowBatterySent) {
    sendDistress(batteryMv, percent, hoursRemaining);
    lowBatterySent = true;
  } else if (trigger) {
    sendDistress(batteryMv, percent, hoursRemaining);
    projectionStageMask |= (1 << stageHit);
  }
}

void BatteryMonitor::sendDistress(uint16_t batteryMv, uint8_t percent,
                                  int32_t hoursRemaining) {
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  char message[PublicChannelAnnouncer::MAX_MESSAGE_LEN];
  BatteryUtils::formatStatus(message, sizeof(message), nodeHash, batteryMv,
                             percent, true, hoursRemaining);

  uint32_t timestamp = TimeSync::now();
  PublicChannelAnnouncer::getInstance().sendText(message, timestamp);
}

void BatteryMonitor::resetProjection() {
  projectionStageMask = 0;
  lowBatterySent = false;
}

bool BatteryMonitor::shouldTriggerThreshold(int32_t hoursRemaining,
                                            uint8_t &triggeredStage) {
  if (hoursRemaining <= 0) {
    return false;
  }
  for (uint8_t i = 0; i < PROJECTION_STEP_COUNT; ++i) {
    uint8_t bit = (1 << i);
    if ((projectionStageMask & bit) == 0 &&
        hoursRemaining <= PROJECTION_STEPS[i]) {
      triggeredStage = i;
      return true;
    }
  }
  return false;
}
