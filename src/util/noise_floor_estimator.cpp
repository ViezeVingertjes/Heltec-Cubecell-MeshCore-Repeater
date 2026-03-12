#include "util/noise_floor_estimator.h"
#include <algorithm>
#include <cmath>
namespace MiniCore {
NoiseFloorEstimator::NoiseFloorEstimator(const Config& config)
    : config_(config)
{
}
void NoiseFloorEstimator::addSample(int16_t rssiDbm) {
    float sample = static_cast<float>(rssiDbm);
    if (sampleCount_ == 0) {
        noiseFloor_ = sample;
        variance_ = 0.0f;
    } else {
        float delta = sample - noiseFloor_;
        noiseFloor_ = noiseFloor_ + config_.ewmaAlpha * delta;
        float squaredDelta = delta * delta;
        variance_ = variance_ + config_.varianceAlpha * (squaredDelta - variance_);
    }
    ++sampleCount_;
}
void NoiseFloorEstimator::reset() {
    noiseFloor_ = -120.0f;
    variance_ = 0.0f;
    sampleCount_ = 0;
}
int16_t NoiseFloorEstimator::getNoiseFloor() const {
    return static_cast<int16_t>(std::round(noiseFloor_));
}
int16_t NoiseFloorEstimator::getThreshold() const {
    float stdDev = std::sqrt(variance_);
    float margin = std::max(config_.sigmaMultiplier * stdDev,
                            static_cast<float>(config_.minMarginDb));
    return static_cast<int16_t>(std::round(noiseFloor_ + margin));
}
bool NoiseFloorEstimator::isAboveThreshold(int16_t rssi) const {
    if (!isValid()) {
        return false;
    }
    return rssi > getThreshold();
}
bool NoiseFloorEstimator::isValid() const {
    return sampleCount_ >= config_.minSamples;
}
uint16_t NoiseFloorEstimator::sampleCount() const {
    return sampleCount_;
}
}
