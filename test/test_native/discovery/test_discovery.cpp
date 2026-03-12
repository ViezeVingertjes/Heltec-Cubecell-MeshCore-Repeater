#include <unity.h>
#include "discovery/discovery.h"
#include "crypto/crypto_types.h"
#include <cstring>
using namespace MiniCore;
void test_parse_discover_request_valid() {
    uint8_t payload[10] = {0x80, 0x04, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00};
    DiscoverRequest req;
        auto status = parseDiscoverRequest(payload, 10, req);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_HEX8(0x04, req.filter);
    TEST_ASSERT_EQUAL_HEX32(0x78563412, req.tag);
    TEST_ASSERT_FALSE(req.prefixOnly);
}
void test_parse_discover_request_prefix_only() {
    uint8_t payload[6] = {0x81, 0x04, 0x12, 0x34, 0x56, 0x78};
    DiscoverRequest req;
        auto status = parseDiscoverRequest(payload, 6, req);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_TRUE(req.prefixOnly);
}
void test_parse_discover_request_too_short() {
    uint8_t payload[5] = {0x80, 0x04, 0x12, 0x34, 0x56};
    DiscoverRequest req;
        auto status = parseDiscoverRequest(payload, 5, req);
        TEST_ASSERT_TRUE(status.isError());
}
void test_parse_discover_request_wrong_type() {
    uint8_t payload[6] = {0x90, 0x04, 0x12, 0x34, 0x56, 0x78};
    DiscoverRequest req;
        auto status = parseDiscoverRequest(payload, 6, req);
        TEST_ASSERT_TRUE(status.isError());
}
void test_parse_discover_request_null_payload() {
    DiscoverRequest req;
        auto status = parseDiscoverRequest(nullptr, 6, req);
        TEST_ASSERT_TRUE(status.isError());
}
void test_matches_node_type_repeater() {
    DiscoverRequest req;
    req.filter = 0x04;
        TEST_ASSERT_TRUE(matchesNodeType(req, ADV_TYPE_REPEATER));
}
void test_matches_node_type_no_match() {
    DiscoverRequest req;
    req.filter = 0x01;
        TEST_ASSERT_FALSE(matchesNodeType(req, ADV_TYPE_REPEATER));
}
void test_create_discover_response_full_key() {
    LocalIdentity identity;
    std::memset(identity.publicKey.bytes, 0xAB, PUB_KEY_SIZE);
        uint32_t tag = 0x12345678;
    int8_t snr = 10;
        uint8_t buffer[64];
    uint8_t len = 0;
        auto status = createDiscoverResponse(identity, tag, snr, false, buffer, len);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(6 + PUB_KEY_SIZE, len);
    TEST_ASSERT_EQUAL_HEX8(0x90 | ADV_TYPE_REPEATER, buffer[0]);
    TEST_ASSERT_EQUAL_INT8(snr, static_cast<int8_t>(buffer[1]));
        uint32_t respTag;
    std::memcpy(&respTag, &buffer[2], 4);
    TEST_ASSERT_EQUAL_HEX32(tag, respTag);
        TEST_ASSERT_EQUAL_HEX8(0xAB, buffer[6]);
}
void test_create_discover_response_prefix_only() {
    LocalIdentity identity;
    std::memset(identity.publicKey.bytes, 0xCD, PUB_KEY_SIZE);
        uint32_t tag = 0xDEADBEEF;
    int8_t snr = -5;
        uint8_t buffer[64];
    uint8_t len = 0;
        auto status = createDiscoverResponse(identity, tag, snr, true, buffer, len);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(6 + 8, len);
    TEST_ASSERT_EQUAL_HEX8(0xCD, buffer[6]);
}
void test_create_discover_response_null_buffer() {
    LocalIdentity identity;
    uint8_t len = 0;
        auto status = createDiscoverResponse(identity, 0, 0, false, nullptr, len);
        TEST_ASSERT_TRUE(status.isError());
}
