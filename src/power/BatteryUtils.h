#pragma once

#include <stdint.h>
#include <stddef.h>

namespace BatteryUtils {

uint8_t estimatePercent(uint16_t batteryMv);
inline bool isUsbPowered(uint16_t batteryMv) { return batteryMv <= 10; }
void formatStatus(char *dest, size_t len, uint8_t nodeHash, uint16_t batteryMv,
                  uint8_t percent, bool lowWarning, int32_t hoursRemaining);
void formatUptime(char *dest, size_t len, uint32_t milliseconds);

}

