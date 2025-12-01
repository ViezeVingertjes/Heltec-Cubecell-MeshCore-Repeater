#pragma once

#include <stdint.h>

namespace BatteryEstimator {

void reset();
void addSample(uint32_t timestampSec, uint8_t percent);
bool estimateHours(int32_t &hoursRemaining);

}

