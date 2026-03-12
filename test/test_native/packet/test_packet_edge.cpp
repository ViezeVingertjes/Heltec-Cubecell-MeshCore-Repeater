#include <unity.h>
#include <cstring>
#include "core/config.h"
#include "packet/packet.h"
#include "routing/repeater.h"
#include "core/result.h"
using namespace MiniCore;

void test_encode_fails_null_destination() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0xAB;
    uint8_t len = 0;
    auto status = encodePacket(pkt, nullptr, len);
    TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, status.error());
}
void test_encode_fails_path_too_long() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Ack);
    pkt.pathLen = MAX_PATH_SIZE + 1;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0xAB;
    uint8_t dest[MAX_MTU_SIZE];
    uint8_t len = 0;
    auto status = encodePacket(pkt, dest, len);
    TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::BufferOverflow, status.error());
}
void test_encode_fails_payload_too_long() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 0;
    pkt.payloadLen = MAX_PACKET_PAYLOAD + 1;
    uint8_t dest[MAX_MTU_SIZE + 10];
    uint8_t len = 0;
    auto status = encodePacket(pkt, dest, len);
    TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::BufferOverflow, status.error());
}
void test_remove_self_from_path_empty() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    Packet pkt;
    pkt.pathLen = 0;
    repeater.removeSelfFromPath(pkt);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.pathLen);
}
void test_append_snr_to_path_at_max() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    Packet pkt;
    pkt.pathLen = MAX_PATH_SIZE;
    pkt.snr = 10;
    repeater.appendSnrToPath(pkt);
    TEST_ASSERT_EQUAL_UINT8(MAX_PATH_SIZE, pkt.pathLen);
}
void test_extract_partial_packet_info_from_valid_header() {
    uint8_t data[] = {0x09, 0x00, 0x05};
    Packet pkt;
    Status status = decodePacket(data, sizeof(data), pkt);
    PartialPacketInfo info = extractPartialPacketInfo(data, sizeof(data), status);
    TEST_ASSERT_TRUE(info.hasHeader);
    TEST_ASSERT_EQUAL(RouteType::Flood, info.routeType);
    TEST_ASSERT_EQUAL(PayloadType::TextMessage, info.payloadType);
    TEST_ASSERT_FALSE(info.hasTransportCodes);
    TEST_ASSERT_EQUAL_UINT8(0, info.pathLen);
    TEST_ASSERT_FALSE(status.isError());
}
void test_extract_partial_packet_info_from_decode_error() {
    uint8_t data[] = {0x52, 0x00};
    Packet pkt;
    Status status = decodePacket(data, sizeof(data), pkt);
    TEST_ASSERT_TRUE(status.isError());
    PartialPacketInfo info = extractPartialPacketInfo(data, sizeof(data), status);
    TEST_ASSERT_TRUE(info.hasHeader);
    TEST_ASSERT_EQUAL(RouteType::Direct, info.routeType);
    TEST_ASSERT_EQUAL(PayloadType::Advert, info.payloadType);
    TEST_ASSERT_EQUAL(status.error(), info.decodeError);
}
void test_extract_partial_packet_info_with_transport_codes() {
    uint8_t data[] = {0x00, 0x12, 0x34, 0x56, 0x78, 0x02, 0x05};
    Packet pkt;
    Status status = decodePacket(data, sizeof(data), pkt);
    PartialPacketInfo info = extractPartialPacketInfo(data, sizeof(data), status);
    TEST_ASSERT_TRUE(info.hasHeader);
    TEST_ASSERT_EQUAL(RouteType::TransportFlood, info.routeType);
    TEST_ASSERT_TRUE(info.hasTransportCodes);
    TEST_ASSERT_EQUAL_UINT8(2, info.pathLen);
}
void test_extract_partial_packet_info_empty_buffer() {
    PartialPacketInfo info = extractPartialPacketInfo(nullptr, 0, ErrorCode::InvalidParameter);
    TEST_ASSERT_FALSE(info.hasHeader);
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, info.decodeError);
}
void test_extract_partial_packet_info_too_short() {
    uint8_t data[] = {0x09};
    Packet pkt;
    Status status = decodePacket(data, sizeof(data), pkt);
    PartialPacketInfo info = extractPartialPacketInfo(data, sizeof(data), status);
    TEST_ASSERT_TRUE(info.hasHeader);
    TEST_ASSERT_EQUAL(RouteType::Flood, info.routeType);
    TEST_ASSERT_EQUAL(PayloadType::TextMessage, info.payloadType);
    TEST_ASSERT_TRUE(status.isError());
}
void test_decode_fails_too_many_hops() {
    constexpr uint8_t PATH_LEN = 33;
    uint8_t raw[2 + PATH_LEN + 1];
    raw[0] = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    raw[1] = PATH_LEN;
    for (uint8_t i = 0; i < PATH_LEN; ++i) {
        raw[2 + i] = static_cast<uint8_t>(0x10 + i);
    }
    raw[2 + PATH_LEN] = 0xAB;
    Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
    TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::TooManyHops, status.error());
}
void test_decode_succeeds_at_max_hops() {
    constexpr uint8_t PATH_LEN = 32;
    uint8_t raw[2 + PATH_LEN + 1];
    raw[0] = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    raw[1] = PATH_LEN;
    for (uint8_t i = 0; i < PATH_LEN; ++i) {
        raw[2 + i] = static_cast<uint8_t>(0x10 + i);
    }
    raw[2 + PATH_LEN] = 0xAB;
    Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
    TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(32, pkt.pathLen);
}
void test_decode_succeeds_under_max_hops() {
    constexpr uint8_t PATH_LEN = 31;
    uint8_t raw[2 + PATH_LEN + 1];
    raw[0] = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    raw[1] = PATH_LEN;
    for (uint8_t i = 0; i < PATH_LEN; ++i) {
        raw[2 + i] = static_cast<uint8_t>(0x10 + i);
    }
    raw[2 + PATH_LEN] = 0xAB;
    Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
    TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(31, pkt.pathLen);
}
void test_decode_fails_too_many_hops_with_transport_codes() {
    constexpr uint8_t PATH_LEN = 33;
    uint8_t raw[1 + 4 + 1 + PATH_LEN + 1];
    raw[0] = Packet::makeHeader(RouteType::TransportFlood, PayloadType::TextMessage);
    raw[1] = 0x12;
    raw[2] = 0x34;
    raw[3] = 0x56;
    raw[4] = 0x78;
    raw[5] = PATH_LEN;
    for (uint8_t i = 0; i < PATH_LEN; ++i) {
        raw[6 + i] = static_cast<uint8_t>(0x10 + i);
    }
    raw[6 + PATH_LEN] = 0xAB;
    Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
    TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::TooManyHops, status.error());
}
