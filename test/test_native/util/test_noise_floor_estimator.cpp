#include <unity.h>
#include "util/noise_floor_estimator.h"
#include "hal/i_radio.h"
using namespace MiniCore;

void test_noise_floor_estimator_initial_state_is_invalid() {
    NoiseFloorEstimator estimator;
        TEST_ASSERT_FALSE(estimator.isValid());
    TEST_ASSERT_EQUAL(0, estimator.sampleCount());
}
void test_noise_floor_estimator_becomes_valid_after_min_samples() {
    NoiseFloorEstimator estimator;
    auto config = NoiseFloorEstimator::Config::defaults();

    for (uint8_t i = 0; i < config.minSamples - 1; ++i) {
        estimator.addSample(-100);
        TEST_ASSERT_FALSE(estimator.isValid());
    }

    estimator.addSample(-100);
    TEST_ASSERT_TRUE(estimator.isValid());
}

void test_noise_floor_estimator_converges_to_stable_value() {
    NoiseFloorEstimator estimator;

    for (int i = 0; i < 50; ++i) {
        estimator.addSample(-95);
    }

    int16_t noiseFloor = estimator.getNoiseFloor();
    TEST_ASSERT_INT16_WITHIN(2, -95, noiseFloor);
}
void test_noise_floor_estimator_ewma_smooths_values() {
    NoiseFloorEstimator estimator;

    for (int i = 0; i < 20; ++i) {
        estimator.addSample(-100);
    }

    estimator.addSample(-50);

    int16_t noiseFloor = estimator.getNoiseFloor();
    TEST_ASSERT_TRUE(noiseFloor < -80);
}
void test_noise_floor_estimator_adapts_to_changing_environment() {
    NoiseFloorEstimator estimator;

    for (int i = 0; i < 30; ++i) {
        estimator.addSample(-110);
    }
        int16_t initial = estimator.getNoiseFloor();
    TEST_ASSERT_INT16_WITHIN(3, -110, initial);

    for (int i = 0; i < 100; ++i) {
        estimator.addSample(-85);
    }
        int16_t adapted = estimator.getNoiseFloor();
    TEST_ASSERT_INT16_WITHIN(3, -85, adapted);
}

void test_noise_floor_estimator_threshold_above_noise_floor() {
    NoiseFloorEstimator estimator;
        for (int i = 0; i < 50; ++i) {
        estimator.addSample(-100);
    }
        int16_t noiseFloor = estimator.getNoiseFloor();
    int16_t threshold = estimator.getThreshold();

    TEST_ASSERT_TRUE(threshold > noiseFloor);
}
void test_noise_floor_estimator_threshold_based_on_variance() {
    NoiseFloorEstimator estimator;

    for (int i = 0; i < 50; ++i) {
        estimator.addSample(-100);
    }
    int16_t lowVarianceThreshold = estimator.getThreshold();
    int16_t lowVarianceFloor = estimator.getNoiseFloor();
    int16_t lowMargin = lowVarianceThreshold - lowVarianceFloor;

    NoiseFloorEstimator estimator2;
    for (int i = 0; i < 50; ++i) {

        estimator2.addSample((i % 2 == 0) ? -90 : -110);
    }
    int16_t highVarianceThreshold = estimator2.getThreshold();
    int16_t highVarianceFloor = estimator2.getNoiseFloor();
    int16_t highMargin = highVarianceThreshold - highVarianceFloor;

    TEST_ASSERT_TRUE(highMargin > lowMargin);
}
void test_noise_floor_estimator_minimum_margin_applied() {
    NoiseFloorEstimatorConfig config = NoiseFloorEstimatorConfig::defaults();
    config.minMarginDb = 10;
    NoiseFloorEstimator estimator(config);

    for (int i = 0; i < 50; ++i) {
        estimator.addSample(-100);
    }
        int16_t noiseFloor = estimator.getNoiseFloor();
    int16_t threshold = estimator.getThreshold();

    TEST_ASSERT_TRUE(threshold - noiseFloor >= config.minMarginDb);
}

void test_noise_floor_estimator_detects_signal_above_threshold() {
    NoiseFloorEstimator estimator;
        for (int i = 0; i < 50; ++i) {
        estimator.addSample(-100);
    }
        int16_t threshold = estimator.getThreshold();

    TEST_ASSERT_TRUE(estimator.isAboveThreshold(threshold + 10));
    TEST_ASSERT_TRUE(estimator.isAboveThreshold(-50));
}
void test_noise_floor_estimator_ignores_signal_below_threshold() {
    NoiseFloorEstimator estimator;
        for (int i = 0; i < 50; ++i) {
        estimator.addSample(-100);
    }
        int16_t noiseFloor = estimator.getNoiseFloor();

    TEST_ASSERT_FALSE(estimator.isAboveThreshold(noiseFloor));
    TEST_ASSERT_FALSE(estimator.isAboveThreshold(noiseFloor - 10));
}
void test_noise_floor_estimator_invalid_always_returns_false() {
    NoiseFloorEstimator estimator;

    TEST_ASSERT_FALSE(estimator.isAboveThreshold(-50));
    TEST_ASSERT_FALSE(estimator.isAboveThreshold(0));
}

void test_noise_floor_estimator_respects_custom_min_samples() {
    NoiseFloorEstimatorConfig config = NoiseFloorEstimatorConfig::defaults();
    config.minSamples = 5;
    NoiseFloorEstimator estimator(config);
        for (int i = 0; i < 4; ++i) {
        estimator.addSample(-100);
    }
    TEST_ASSERT_FALSE(estimator.isValid());
        estimator.addSample(-100);
    TEST_ASSERT_TRUE(estimator.isValid());
}
void test_noise_floor_estimator_respects_custom_alpha() {

    NoiseFloorEstimatorConfig fastConfig = NoiseFloorEstimatorConfig::defaults();
    fastConfig.ewmaAlpha = 0.5f;
    fastConfig.minSamples = 1;
    NoiseFloorEstimator fastEstimator(fastConfig);

    NoiseFloorEstimatorConfig slowConfig = NoiseFloorEstimatorConfig::defaults();
    slowConfig.ewmaAlpha = 0.05f;
    slowConfig.minSamples = 1;
    NoiseFloorEstimator slowEstimator(slowConfig);

    fastEstimator.addSample(-100);
    slowEstimator.addSample(-100);

    for (int i = 0; i < 10; ++i) {
        fastEstimator.addSample(-80);
        slowEstimator.addSample(-80);
    }

    int16_t fastFloor = fastEstimator.getNoiseFloor();
    int16_t slowFloor = slowEstimator.getNoiseFloor();
        TEST_ASSERT_TRUE(fastFloor > slowFloor);
}
void test_noise_floor_estimator_default_config() {
    NoiseFloorEstimatorConfig config = NoiseFloorEstimatorConfig::defaults();
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.1f, config.ewmaAlpha);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.1f, config.varianceAlpha);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 3.0f, config.sigmaMultiplier);
    TEST_ASSERT_EQUAL(10, config.minSamples);
    TEST_ASSERT_EQUAL(6, config.minMarginDb);
}

void test_noise_floor_estimator_reset_clears_state() {
    NoiseFloorEstimator estimator;
        for (int i = 0; i < 50; ++i) {
        estimator.addSample(-90);
    }
    TEST_ASSERT_TRUE(estimator.isValid());
        estimator.reset();
        TEST_ASSERT_FALSE(estimator.isValid());
    TEST_ASSERT_EQUAL(0, estimator.sampleCount());
}

void test_noise_floor_estimator_handles_extreme_rssi_values() {
    NoiseFloorEstimator estimator;

    for (int i = 0; i < 20; ++i) {
        estimator.addSample(-130);
    }
    TEST_ASSERT_INT16_WITHIN(5, -130, estimator.getNoiseFloor());
        estimator.reset();

    for (int i = 0; i < 20; ++i) {
        estimator.addSample(-20);
    }
    TEST_ASSERT_INT16_WITHIN(5, -20, estimator.getNoiseFloor());
}
void test_noise_floor_estimator_sample_count_increments() {
    NoiseFloorEstimator estimator;
        TEST_ASSERT_EQUAL(0, estimator.sampleCount());
        estimator.addSample(-100);
    TEST_ASSERT_EQUAL(1, estimator.sampleCount());
        estimator.addSample(-100);
    TEST_ASSERT_EQUAL(2, estimator.sampleCount());
}

void test_margin_for_bandwidth_narrow_returns_small_margin() {

    TEST_ASSERT_EQUAL(3, marginForBandwidth(LoraBandwidth::BW_7_8));
    TEST_ASSERT_EQUAL(3, marginForBandwidth(LoraBandwidth::BW_10_4));
    TEST_ASSERT_EQUAL(3, marginForBandwidth(LoraBandwidth::BW_15_6));
    TEST_ASSERT_EQUAL(3, marginForBandwidth(LoraBandwidth::BW_20_8));
    TEST_ASSERT_EQUAL(3, marginForBandwidth(LoraBandwidth::BW_31_25));
}
void test_margin_for_bandwidth_medium_returns_default_margin() {

    TEST_ASSERT_EQUAL(6, marginForBandwidth(LoraBandwidth::BW_41_7));
    TEST_ASSERT_EQUAL(6, marginForBandwidth(LoraBandwidth::BW_62_5));
}
void test_margin_for_bandwidth_wide_returns_larger_margin() {

    TEST_ASSERT_EQUAL(9, marginForBandwidth(LoraBandwidth::BW_125));
    TEST_ASSERT_EQUAL(12, marginForBandwidth(LoraBandwidth::BW_250));
    TEST_ASSERT_EQUAL(15, marginForBandwidth(LoraBandwidth::BW_500));
}
void test_noise_floor_config_for_bandwidth() {

    auto config = NoiseFloorEstimatorConfig::forBandwidth(LoraBandwidth::BW_125);
    TEST_ASSERT_EQUAL(9, config.minMarginDb);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.1f, config.ewmaAlpha);
    TEST_ASSERT_EQUAL(10, config.minSamples);
}
void test_noise_floor_estimator_with_bandwidth_config() {
    auto config = NoiseFloorEstimatorConfig::forBandwidth(LoraBandwidth::BW_500);
    NoiseFloorEstimator estimator(config);

    for (int i = 0; i < 50; ++i) {
        estimator.addSample(-100);
    }
        int16_t noiseFloor = estimator.getNoiseFloor();
    int16_t threshold = estimator.getThreshold();

    TEST_ASSERT_TRUE(threshold - noiseFloor >= 15);
}
