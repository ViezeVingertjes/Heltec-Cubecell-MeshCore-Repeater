#include <unity.h>
#include "advert/advert.h"
#include "crypto/crypto_types.h"
#include "mocks/mock_crypto.h"
#include <cstring>
using namespace MiniCore;
using MiniCore::Mocks::MockCrypto;
void test_encode_advert_data_type_only() {
    uint8_t buffer[MAX_ADVERT_DATA_SIZE];
    uint8_t len = encodeAdvertData(ADV_TYPE_REPEATER, nullptr, buffer);
        TEST_ASSERT_EQUAL_UINT8(1, len);
    TEST_ASSERT_EQUAL_HEX8(ADV_TYPE_REPEATER, buffer[0] & 0x0F);
}
void test_encode_advert_data_with_name() {
    uint8_t buffer[MAX_ADVERT_DATA_SIZE];
    const char* name = "MyRepeater";
    uint8_t len = encodeAdvertData(ADV_TYPE_REPEATER, name, buffer);
        TEST_ASSERT_EQUAL_UINT8(1 + 10, len);
    TEST_ASSERT_EQUAL_HEX8(ADV_TYPE_REPEATER | ADV_NAME_MASK, buffer[0]);
    TEST_ASSERT_EQUAL_MEMORY("MyRepeater", &buffer[1], 10);
}
void test_encode_advert_data_name_truncated() {
    uint8_t buffer[MAX_ADVERT_DATA_SIZE];
    const char* longName = "ThisNameIsMuchTooLongToFit";
    uint8_t len = encodeAdvertData(ADV_TYPE_REPEATER, longName, buffer);
        TEST_ASSERT_TRUE(len <= MAX_ADVERT_DATA_SIZE);
    TEST_ASSERT_EQUAL_HEX8(ADV_TYPE_REPEATER | ADV_NAME_MASK, buffer[0]);
}
void test_encode_advert_data_null_name_no_flag() {
    uint8_t buffer[MAX_ADVERT_DATA_SIZE];
    (void)encodeAdvertData(ADV_TYPE_REPEATER, nullptr, buffer);
        TEST_ASSERT_FALSE(buffer[0] & ADV_NAME_MASK);
}
void test_encode_advert_data_empty_name_no_flag() {
    uint8_t buffer[MAX_ADVERT_DATA_SIZE];
    (void)encodeAdvertData(ADV_TYPE_REPEATER, "", buffer);
        TEST_ASSERT_FALSE(buffer[0] & ADV_NAME_MASK);
}
void test_create_advert_packet_structure() {
    LocalIdentity identity;
    std::memset(identity.publicKey.bytes, 0xAB, PUB_KEY_SIZE);
    std::memset(identity.privateKey.bytes, 0xCD, PRV_KEY_SIZE);
        MockCrypto crypto;
    uint32_t timestamp = 1700000000;
    const char* name = "Test";
        Packet pkt;
    auto status = createAdvertPacket(identity, timestamp, name, crypto, pkt);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL(PayloadType::Advert, pkt.getPayloadType());
    TEST_ASSERT_EQUAL(RouteType::Flood, pkt.getRouteType());
    TEST_ASSERT_EQUAL_UINT8(0, pkt.pathLen);
        TEST_ASSERT_EQUAL_HEX8(0xAB, pkt.payload[0]);
        uint32_t pktTimestamp;
    std::memcpy(&pktTimestamp, &pkt.payload[PUB_KEY_SIZE], 4);
    TEST_ASSERT_EQUAL_UINT32(timestamp, pktTimestamp);
}
void test_create_advert_packet_has_signature() {
    LocalIdentity identity;
    std::memset(identity.publicKey.bytes, 0xAB, PUB_KEY_SIZE);
    std::memset(identity.privateKey.bytes, 0xCD, PRV_KEY_SIZE);
        MockCrypto crypto;
    crypto.setSignaturePattern(0x55);
        Packet pkt;
    auto status = createAdvertPacket(identity, 1700000000, nullptr, crypto, pkt);
        TEST_ASSERT_TRUE(status.isOk());
        size_t sigOffset = PUB_KEY_SIZE + 4;
    TEST_ASSERT_EQUAL_HEX8(0x55, pkt.payload[sigOffset]);
}
void test_create_advert_packet_payload_length() {
    LocalIdentity identity;
    MockCrypto crypto;
        Packet pkt;
    auto status = createAdvertPacket(identity, 1700000000, "Test", crypto, pkt);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(PUB_KEY_SIZE + 4 + SIGNATURE_SIZE + 1 + 4, pkt.payloadLen);
}
void test_advert_scheduler_not_due_before_time_sync() {
    AdvertScheduler scheduler;
        TEST_ASSERT_FALSE(scheduler.isDue(1000, false));
}
void test_advert_scheduler_not_due_if_recently_sent() {
    AdvertScheduler scheduler;
    scheduler.setLastAdvertTime(1000);
        uint32_t currentTime = 1000 + ADVERT_INTERVAL_SECONDS - 1;
    TEST_ASSERT_FALSE(scheduler.isDue(currentTime, true));
}
void test_advert_scheduler_due_after_interval() {
    AdvertScheduler scheduler;
    scheduler.setLastAdvertTime(1000);
        uint32_t currentTime = 1000 + ADVERT_INTERVAL_SECONDS + 1;
    TEST_ASSERT_TRUE(scheduler.isDue(currentTime, true));
}
void test_advert_scheduler_due_if_never_sent() {
    AdvertScheduler scheduler;
        TEST_ASSERT_TRUE(scheduler.isDue(1000, true));
}
void test_advert_scheduler_mark_sent_updates_time() {
    AdvertScheduler scheduler;
    scheduler.markSent(5000);
        TEST_ASSERT_EQUAL_UINT32(5000, scheduler.lastAdvertTime());
    TEST_ASSERT_FALSE(scheduler.isDue(5000 + 100, true));
}
void test_advert_scheduler_persist_and_restore() {
    AdvertScheduler scheduler1;
    scheduler1.setLastAdvertTime(1700000000);
        uint8_t buffer[ADVERT_STATE_STORAGE_SIZE];
    scheduler1.saveTo(buffer);
        AdvertScheduler scheduler2;
    scheduler2.loadFrom(buffer);
        TEST_ASSERT_EQUAL_UINT32(1700000000, scheduler2.lastAdvertTime());
}
void test_advert_scheduler_empty_storage_returns_zero() {
    AdvertScheduler scheduler;
    uint8_t buffer[ADVERT_STATE_STORAGE_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF};
        scheduler.loadFrom(buffer);
        TEST_ASSERT_EQUAL_UINT32(0, scheduler.lastAdvertTime());
}
void test_advert_scheduler_not_due_if_time_moved_backwards() {
    AdvertScheduler scheduler;
    scheduler.setLastAdvertTime(5000);

    uint32_t pastTime = 4000;
    TEST_ASSERT_FALSE(scheduler.isDue(pastTime, true));

    TEST_ASSERT_FALSE(scheduler.isDue(5000, true));
}
