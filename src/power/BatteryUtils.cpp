#include "BatteryUtils.h"
#include "PowerManager.h"
#include <stdio.h>

namespace BatteryUtils {

uint8_t estimatePercent(uint16_t batteryMv) {
  static const struct {
    uint16_t mv;
    uint8_t percent;
  } curve[] = {
      {3600, 100}, {3450, 80}, {3350, 60}, {3250, 40},
      {3150, 20},  {3000, 5},  {2900, 0},
  };

  if (batteryMv >= curve[0].mv) {
    return 100;
  }

  const size_t entries = sizeof(curve) / sizeof(curve[0]);
  for (size_t i = 0; i < entries - 1; ++i) {
    auto high = curve[i];
    auto low = curve[i + 1];
    if (batteryMv >= low.mv) {
      float span = static_cast<float>(high.mv - low.mv);
      float pos = static_cast<float>(batteryMv - low.mv);
      float pct = low.percent;
      if (span > 0.0f) {
        pct += (high.percent - low.percent) * (pos / span);
      }
      if (pct < 0.0f) pct = 0.0f;
      if (pct > 100.0f) pct = 100.0f;
      return static_cast<uint8_t>(pct + 0.5f);
    }
  }
  return 0;
}

void formatUptime(char *dest, size_t len, uint32_t milliseconds) {
  // Convert to seconds, avoiding overflow
  uint32_t totalSeconds = milliseconds / 1000;
  
  uint32_t seconds = totalSeconds % 60;
  uint32_t totalMinutes = totalSeconds / 60;
  uint32_t minutes = totalMinutes % 60;
  uint32_t totalHours = totalMinutes / 60;
  uint32_t hours = totalHours % 24;
  uint32_t days = totalHours / 24;
  
  if (days > 0) {
    snprintf(dest, len, "%lud %luh", days, hours);
  } else if (hours > 0) {
    snprintf(dest, len, "%luh %lum", hours, minutes);
  } else if (minutes > 0) {
    snprintf(dest, len, "%lum %lus", minutes, seconds);
  } else {
    snprintf(dest, len, "%lus", seconds);
  }
}

void formatStatus(char *dest, size_t len, uint8_t nodeHash, uint16_t batteryMv,
                  uint8_t percent, bool lowWarning, int32_t hoursRemaining) {
  // Format runtime estimate
  char runtimeText[32] = "";
  if (hoursRemaining > 0) {
    int32_t totalMinutes = hoursRemaining * 60;
    int32_t days = totalMinutes / (24 * 60);
    int32_t remainingMinutes = totalMinutes % (24 * 60);
    int32_t hours = remainingMinutes / 60;
    int32_t minutes = remainingMinutes % 60;
    if (days > 0) {
      snprintf(runtimeText, sizeof(runtimeText), "%dd %dh remaining", days, hours);
    } else if (hours > 0) {
      snprintf(runtimeText, sizeof(runtimeText), "%dh %dm remaining", hours, minutes);
    } else {
      snprintf(runtimeText, sizeof(runtimeText), "%dm remaining", minutes);
    }
  } else {
    snprintf(runtimeText, sizeof(runtimeText), "collecting data");
  }
  
  // Get wake/sleep statistics
  uint32_t totalUptime = millis();
  uint32_t sleepTime = PowerManager::getInstance().getTotalSleepTime();
  uint32_t wakeTime = totalUptime > sleepTime ? totalUptime - sleepTime : 0;
  
  char wakeText[24];
  char sleepText[24];
  formatUptime(wakeText, sizeof(wakeText), wakeTime);
  formatUptime(sleepText, sizeof(sleepText), sleepTime);
  
  // Format message based on power source
  if (isUsbPowered(batteryMv)) {
    snprintf(dest, len, "Node %02X: USB powered | W:%s S:%s", 
             nodeHash, wakeText, sleepText);
    return;
  }
  
  // Battery-powered messages
  float batteryV = batteryMv / 1000.0f;
  if (lowWarning) {
    snprintf(dest, len, "Node %02X: LOW BAT %.2fV (%u%%) - %s | W:%s S:%s", 
             nodeHash, batteryV, percent, runtimeText, wakeText, sleepText);
  } else {
    snprintf(dest, len, "Node %02X: %.2fV (%u%%) - %s | W:%s S:%s", 
             nodeHash, batteryV, percent, runtimeText, wakeText, sleepText);
  }
}

}

