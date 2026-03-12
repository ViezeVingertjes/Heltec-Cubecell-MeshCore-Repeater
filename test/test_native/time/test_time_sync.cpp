#include <unity.h>
#include "time/time_sync.h"
#include <cstring>
using namespace MiniCore;

void test_time_sync_initial_offset_is_zero() {
    TimeSynchronizer sync;
    TEST_ASSERT_EQUAL_INT32(0, sync.getOffset());
}
void test_time_sync_no_samples_means_no_adjustment() {
    TimeSynchronizer sync;
    uint32_t localTime = 1000;
    TEST_ASSERT_EQUAL_UINT32(1000, sync.adjustedTime(localTime));
}
void test_time_sync_single_sample_sets_offset() {
    TimeSynchronizer sync;

    sync.addSample(0xAA, 1000, 100);

    TEST_ASSERT_EQUAL_INT32(900, sync.getOffset());

    TEST_ASSERT_EQUAL_UINT32(200, sync.adjustedTime(200));
}
void test_time_sync_requires_min_samples_for_consensus() {
    TimeSynchronizer sync;

    for (uint8_t i = 0; i < 7; ++i) {
        sync.addSample(i, 1000, 100);
    }

    TEST_ASSERT_FALSE(sync.hasConsensus());
    TEST_ASSERT_EQUAL_UINT8(7, sync.uniqueSenderCount());
}
void test_time_sync_consensus_with_enough_unique_senders() {
    TimeSynchronizer sync;

    for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, 1000 + i, 100 + i);
    }
        TEST_ASSERT_TRUE(sync.hasConsensus());
    TEST_ASSERT_EQUAL_INT32(900, sync.getOffset());
}
void test_time_sync_duplicate_sender_not_counted_twice() {
    TimeSynchronizer sync;

    sync.addSample(0xAA, 1000, 100);
    sync.addSample(0xAA, 1100, 200);
    sync.addSample(0xAA, 1200, 300);

    TEST_ASSERT_FALSE(sync.hasConsensus());
    TEST_ASSERT_EQUAL_UINT8(1, sync.uniqueSenderCount());
}
void test_time_sync_duplicate_sender_updates_offset() {
    TimeSynchronizer sync;

    sync.addSample(0xAA, 1000, 100);
    TEST_ASSERT_EQUAL_INT32(900, sync.getOffset());

    sync.addSample(0xAA, 2000, 150);
    TEST_ASSERT_EQUAL_INT32(1850, sync.getOffset());
}
void test_time_sync_median_offset_used() {
    TimeSynchronizer sync;

    sync.addSample(0xAA, 1000, 100);
    sync.addSample(0xBB, 1000, 50);
    sync.addSample(0xCC, 1000, 0);

    TEST_ASSERT_EQUAL_INT32(950, sync.getOffset());
}
void test_time_sync_handles_backward_adjustment() {
    TimeSynchronizer sync;

    for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, 1000 + i, 2000 + i);
    }
        TEST_ASSERT_TRUE(sync.hasConsensus());
    TEST_ASSERT_EQUAL_INT32(-1000, sync.getOffset());
    TEST_ASSERT_EQUAL_UINT32(500, sync.adjustedTime(1500));
}
void test_time_sync_handles_power_loss_scenario() {
    TimeSynchronizer sync;

    constexpr uint32_t LOCAL_AFTER_RESET = 1704067200;
    constexpr uint32_t ACTUAL_TIME = 1735689600;
    constexpr int32_t EXPECTED_OFFSET = static_cast<int32_t>(ACTUAL_TIME - LOCAL_AFTER_RESET);

    for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, ACTUAL_TIME + i, LOCAL_AFTER_RESET + i);
    }
        TEST_ASSERT_TRUE(sync.hasConsensus());
    TEST_ASSERT_EQUAL_INT32(EXPECTED_OFFSET, sync.getOffset());
}
void test_time_sync_outlier_rejected() {
    TimeSynchronizer sync;

    sync.addSample(0x01, 1000, 100);
    sync.addSample(0x02, 1000, 100);
    sync.addSample(0x03, 1000, 100);
    sync.addSample(0x04, 1000, 100);
    sync.addSample(0x05, 1000, 100);
    sync.addSample(0x06, 1000, 100);
    sync.addSample(0x07, 1000, 100);
    sync.addSample(0xDD, 9999, 100);
        TEST_ASSERT_TRUE(sync.hasConsensus());

    int32_t offset = sync.getOffset();
    TEST_ASSERT_EQUAL_INT32(900, offset);
}
void test_time_sync_max_senders_limit() {
    TimeSynchronizer sync;

    for (uint8_t i = 0; i < 20; ++i) {
        sync.addSample(i, 1000 + i, 100);
    }

    TEST_ASSERT_TRUE(sync.uniqueSenderCount() <= 16);
    TEST_ASSERT_TRUE(sync.hasConsensus());
}
void test_time_sync_reset_clears_all() {
    TimeSynchronizer sync;
        for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, 1000, 100);
    }
    TEST_ASSERT_TRUE(sync.hasConsensus());
        sync.reset();
        TEST_ASSERT_FALSE(sync.hasConsensus());
    TEST_ASSERT_EQUAL_UINT8(0, sync.uniqueSenderCount());
    TEST_ASSERT_EQUAL_INT32(0, sync.getOffset());
}
void test_time_sync_get_adjusted_time_without_consensus_returns_local() {
    TimeSynchronizer sync;

    sync.addSample(0xAA, 5000, 1000);

    TEST_ASSERT_EQUAL_UINT32(2000, sync.adjustedTime(2000));
}
void test_time_sync_adjusted_time_with_consensus() {
    TimeSynchronizer sync;

    for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, 1000 + i, 100 + i);
    }

    TEST_ASSERT_EQUAL_UINT32(1400, sync.adjustedTime(500));
}

void test_extract_advert_timestamp_valid() {

    uint8_t payload[100];
    std::memset(payload, 0x11, 32);

    payload[32] = 0x78;
    payload[33] = 0x56;
    payload[34] = 0x34;
    payload[35] = 0x12;
        std::memset(&payload[36], 0xAA, 64);
        uint32_t timestamp = 0;
    uint8_t senderHash = 0;
    auto result = extractAdvertTimestamp(payload, 100, timestamp, senderHash);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL_HEX32(0x12345678, timestamp);
    TEST_ASSERT_EQUAL_HEX8(0x11, senderHash);
}
void test_extract_advert_timestamp_too_short() {
    uint8_t payload[50] = {0};
        uint32_t timestamp = 0;
    uint8_t senderHash = 0;
    auto result = extractAdvertTimestamp(payload, 50, timestamp, senderHash);
        TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, result.error());
}
void test_extract_advert_timestamp_null_payload() {
    uint32_t timestamp = 0;
    uint8_t senderHash = 0;
    auto result = extractAdvertTimestamp(nullptr, 100, timestamp, senderHash);
        TEST_ASSERT_TRUE(result.isError());
}
void test_extract_advert_sender_hash_is_first_pubkey_byte() {
    uint8_t payload[100];
    payload[0] = 0xDE;
    std::memset(&payload[1], 0x00, 99);
        uint32_t timestamp = 0;
    uint8_t senderHash = 0;
    auto result = extractAdvertTimestamp(payload, 100, timestamp, senderHash);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL_HEX8(0xDE, senderHash);
}
void test_time_sync_evicts_oldest_when_full() {
    TimeSynchronizer sync;
        for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, 200, 100);
    }
    for (uint8_t i = 8; i < TIME_SYNC_MAX_SENDERS; ++i) {
        sync.addSample(i, 400, 100);
    }
    TEST_ASSERT_EQUAL_UINT8(TIME_SYNC_MAX_SENDERS, sync.uniqueSenderCount());
    TEST_ASSERT_EQUAL_INT32(200, sync.getOffset());
        sync.addSample(0xFF, 600, 100);
        TEST_ASSERT_EQUAL_UINT8(TIME_SYNC_MAX_SENDERS, sync.uniqueSenderCount());
        int32_t offset = sync.getOffset();
    TEST_ASSERT_TRUE(offset > 200);
}
void test_time_sync_eviction_updates_oldest_slot() {
    TimeSynchronizer sync;
        for (uint8_t i = 0; i < TIME_SYNC_MAX_SENDERS; ++i) {
        sync.addSample(i, 1000 + i, 100 + i);
    }
        sync.addSample(0x00, 5000, 500);
        sync.addSample(0xAA, 6000, 600);
        TEST_ASSERT_EQUAL_UINT8(TIME_SYNC_MAX_SENDERS, sync.uniqueSenderCount());
}
void test_time_sync_continuous_adjustment() {
    TimeSynchronizer sync;
        for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, 1000, 100);
    }
    TEST_ASSERT_EQUAL_INT32(900, sync.getOffset());
        for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, 2000, 200);
    }
    TEST_ASSERT_EQUAL_INT32(1800, sync.getOffset());
}
void test_time_sync_uptime_with_unix_timestamp_jan2026() {
    TimeSynchronizer sync;
        constexpr uint32_t JAN_9_2026_UNIX = 1736380800;
    constexpr uint32_t UPTIME_SECONDS = 100;
        for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, JAN_9_2026_UNIX + i, UPTIME_SECONDS + i);
    }
        TEST_ASSERT_TRUE(sync.hasConsensus());
        int32_t offset = sync.getOffset();
    int32_t expectedOffset = static_cast<int32_t>(JAN_9_2026_UNIX - UPTIME_SECONDS);
    TEST_ASSERT_EQUAL_INT32(expectedOffset, offset);
        uint32_t adjustedAt200 = sync.adjustedTime(200);
        uint32_t expectedTime = JAN_9_2026_UNIX + 100;
    TEST_ASSERT_UINT32_WITHIN(10, expectedTime, adjustedAt200);
        TEST_ASSERT_TRUE(adjustedAt200 > 1700000000);
    TEST_ASSERT_TRUE(adjustedAt200 < 1800000000);
}
void test_time_sync_uptime_detects_year_2094_bug() {
    TimeSynchronizer sync;
        constexpr uint32_t JAN_9_2026_UNIX = 1736380800;
    constexpr uint32_t UPTIME_SECONDS = 50;
        for (uint8_t i = 0; i < 8; ++i) {
        sync.addSample(i, JAN_9_2026_UNIX, UPTIME_SECONDS);
    }
        TEST_ASSERT_TRUE(sync.hasConsensus());
        uint32_t adjusted = sync.adjustedTime(100);
        constexpr uint32_t YEAR_2090_UNIX = 3786912000;
    TEST_ASSERT_TRUE(adjusted < YEAR_2090_UNIX);
        constexpr uint32_t YEAR_2020_UNIX = 1577836800;
    TEST_ASSERT_TRUE(adjusted > YEAR_2020_UNIX);
}
