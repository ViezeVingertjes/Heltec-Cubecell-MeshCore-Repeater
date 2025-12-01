#include "BatteryEstimator.h"
#include <cstddef>

namespace {
constexpr size_t MAX_SAMPLES = 36;
constexpr size_t MIN_SAMPLE_COUNT = 12;
constexpr uint32_t MIN_ELAPSED_SEC = 7200; // 2 hours

struct Sample {
  uint32_t timestamp;
  uint8_t percent;
};

Sample samples[MAX_SAMPLES];
size_t sampleCount = 0;
} // namespace

namespace BatteryEstimator {

void reset() {
  sampleCount = 0;
}

void addSample(uint32_t timestampSec, uint8_t percent) {
  if (sampleCount < MAX_SAMPLES) {
    samples[sampleCount++] = {timestampSec, percent};
  } else {
    for (size_t i = 1; i < MAX_SAMPLES; ++i) {
      samples[i - 1] = samples[i];
    }
    samples[MAX_SAMPLES - 1] = {timestampSec, percent};
  }
}

bool estimateHours(int32_t &hoursRemaining) {
  if (sampleCount < MIN_SAMPLE_COUNT) {
    return false;
  }

  const Sample &first = samples[0];
  const Sample &last = samples[sampleCount - 1];
  if (last.timestamp <= first.timestamp || last.percent >= first.percent) {
    return false;
  }

  uint32_t elapsed = last.timestamp - first.timestamp;
  if (elapsed < MIN_ELAPSED_SEC) {
    return false;
  }

  double sumT = 0.0;
  double sumP = 0.0;
  double sumTT = 0.0;
  double sumTP = 0.0;

  for (size_t i = 0; i < sampleCount; ++i) {
    double t = static_cast<double>(samples[i].timestamp - first.timestamp);
    double p = static_cast<double>(samples[i].percent);
    sumT += t;
    sumP += p;
    sumTT += t * t;
    sumTP += t * p;
  }

  double n = static_cast<double>(sampleCount);
  double denom = n * sumTT - sumT * sumT;
  if (denom <= 0.0) {
    return false;
  }
  double slope = (n * sumTP - sumT * sumP) / denom; // percent per second
  if (slope >= 0.0) {
    return false;
  }

  double remainingSeconds = static_cast<double>(last.percent) / (-slope);
  if (remainingSeconds <= 0.0) {
    return false;
  }

  hoursRemaining = static_cast<int32_t>(remainingSeconds / 3600.0 + 0.5);
  return true;
}

}

