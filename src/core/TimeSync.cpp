#include "TimeSync.h"

namespace {
uint32_t syncedEpoch = 0;
uint32_t syncedMillis = 0;
}

namespace TimeSync {

void updateFromRemote(uint32_t remoteSeconds) {
  if (remoteSeconds == 0) {
    return;
  }
  
  // Allow time updates if:
  // 1. We've never synced before, OR
  // 2. Remote time is ahead of us, OR
  // 3. Remote time is behind us by more than 5 seconds (clock drift correction)
  uint32_t currentTime = now();
  int32_t delta = static_cast<int32_t>(remoteSeconds - currentTime);
  
  if (syncedEpoch == 0 || delta > 0 || delta < -5) {
    syncedEpoch = remoteSeconds;
    syncedMillis = millis();
  }
}

uint32_t now() {
  if (syncedEpoch == 0) {
    return millis() / 1000;
  }
  uint32_t elapsedSec = (millis() - syncedMillis) / 1000;
  return syncedEpoch + elapsedSec;
}

}

