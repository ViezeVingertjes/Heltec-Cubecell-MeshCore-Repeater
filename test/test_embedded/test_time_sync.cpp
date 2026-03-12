#include <Arduino.h>
#include <unity.h>
#include "time/time_sync.h"
using namespace MiniCore;
void test_time_sync_consensus_basic() {
    TimeSynchronizer sync;
        for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, 1000 + i, 100 + i);
    }
        TEST_ASSERT_TRUE(sync.hasConsensus());
    TEST_ASSERT_EQUAL_INT32(900, sync.getOffset());
    TEST_ASSERT_EQUAL_UINT32(1400, sync.adjustedTime(500));
}
void test_time_sync_power_loss_recovery() {
    TimeSynchronizer sync;
        constexpr uint32_t LOCAL_TIME = 1704067200;
    constexpr uint32_t ACTUAL_TIME = 1735689600;
        for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, ACTUAL_TIME + i, LOCAL_TIME + i);
    }
        TEST_ASSERT_TRUE(sync.hasConsensus());
        uint32_t adjusted = sync.adjustedTime(LOCAL_TIME + 100);
    TEST_ASSERT_TRUE(adjusted > LOCAL_TIME + 30000000);
}
void test_time_sync_extract_advert() {
    uint8_t payload[100];
        payload[0] = 0xDE;
    for (uint8_t i = 1; i < 32; ++i) {
        payload[i] = i;
    }
        payload[32] = 0x78;
    payload[33] = 0x56;
    payload[34] = 0x34;
    payload[35] = 0x12;
        for (uint8_t i = 36; i < 100; ++i) {
        payload[i] = 0xAA;
    }
        uint32_t timestamp = 0;
    uint8_t senderHash = 0;
    auto result = extractAdvertTimestamp(payload, 100, timestamp, senderHash);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL_HEX32(0x12345678, timestamp);
    TEST_ASSERT_EQUAL_HEX8(0xDE, senderHash);
}
void test_time_sync_reset() {
    TimeSynchronizer sync;
        for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, 1000, 100);
    }
    TEST_ASSERT_TRUE(sync.hasConsensus());
        sync.reset();
        TEST_ASSERT_FALSE(sync.hasConsensus());
    TEST_ASSERT_EQUAL_UINT8(0, sync.uniqueSenderCount());
}
void test_time_sync_eviction() {
    TimeSynchronizer sync;
        for (uint8_t i = 0; i < TIME_SYNC_MAX_SENDERS; ++i) {
        sync.addSample(i, 1000, 100);
    }
    TEST_ASSERT_EQUAL_UINT8(TIME_SYNC_MAX_SENDERS, sync.uniqueSenderCount());
        sync.addSample(0xFF, 2000, 100);
        TEST_ASSERT_EQUAL_UINT8(TIME_SYNC_MAX_SENDERS, sync.uniqueSenderCount());
}
