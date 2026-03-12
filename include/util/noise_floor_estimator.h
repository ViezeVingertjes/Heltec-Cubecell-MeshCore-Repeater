#pragma once
#include <cstdint>
#include "core/config.h"
#include "hal/i_radio.h"
namespace MiniCore {
inline int8_t marginForBandwidth(LoraBandwidth bw) {
    switch (bw) {
        case LoraBandwidth::BW_7_8:
        case LoraBandwidth::BW_10_4:
        case LoraBandwidth::BW_15_6:
        case LoraBandwidth::BW_20_8:
        case LoraBandwidth::BW_31_25:
            return 3;
        case LoraBandwidth::BW_41_7:
        case LoraBandwidth::BW_62_5:
            return 6;
        case LoraBandwidth::BW_125:
            return 9;
        case LoraBandwidth::BW_250:
            return 12;
        case LoraBandwidth::BW_500:
            return 15;
    }
    return Config::NOISE_FLOOR_MIN_MARGIN_DB;
}
struct NoiseFloorEstimatorConfig {
    float ewmaAlpha;
    float varianceAlpha;
    float sigmaMultiplier;
    uint8_t minSamples;
    int8_t minMarginDb;
    static NoiseFloorEstimatorConfig defaults() {
        return {
            .ewmaAlpha = Config::NOISE_FLOOR_EWMA_ALPHA,
            .varianceAlpha = Config::NOISE_FLOOR_VARIANCE_ALPHA,
            .sigmaMultiplier = Config::NOISE_FLOOR_SIGMA_MULTIPLIER,
            .minSamples = Config::NOISE_FLOOR_MIN_SAMPLES,
            .minMarginDb = Config::NOISE_FLOOR_MIN_MARGIN_DB
        };
    }
    static NoiseFloorEstimatorConfig forBandwidth(LoraBandwidth bw) {
        auto config = defaults();
        config.minMarginDb = marginForBandwidth(bw);
        return config;
    }
};
class NoiseFloorEstimator {
public:
    using Config = NoiseFloorEstimatorConfig;
    NoiseFloorEstimator() = default;
    explicit NoiseFloorEstimator(const Config& config);
    void addSample(int16_t rssiDbm);
    void reset();
    [[nodiscard]] int16_t getNoiseFloor() const;
    [[nodiscard]] int16_t getThreshold() const;
    [[nodiscard]] bool isAboveThreshold(int16_t rssi) const;
    [[nodiscard]] bool isValid() const;
    [[nodiscard]] uint16_t sampleCount() const;
private:
    Config config_{Config::defaults()};
    float noiseFloor_{-120.0f};
    float variance_{0.0f};
    uint16_t sampleCount_{0};
};
}
