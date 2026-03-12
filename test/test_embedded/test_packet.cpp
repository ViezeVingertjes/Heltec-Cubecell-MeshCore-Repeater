#include <Arduino.h>
#include <unity.h>
#include "packet/packet.h"
using namespace MiniCore;

void test_packet_construct_and_decode() {

    uint8_t raw[] = {
        Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage),
        0x02,
        0xAA, 0xBB,
        0x48, 0x69
    };
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL(RouteType::Flood, pkt.getRouteType());
    TEST_ASSERT_EQUAL(PayloadType::TextMessage, pkt.getPayloadType());
    TEST_ASSERT_EQUAL_UINT8(2, pkt.pathLen);
    TEST_ASSERT_EQUAL_UINT16(2, pkt.payloadLen);
    TEST_ASSERT_EQUAL_HEX8(0x48, pkt.payload[0]);
    TEST_ASSERT_EQUAL_HEX8(0x69, pkt.payload[1]);
}
void test_packet_encode_decode_roundtrip() {
    Packet original;
    original.header = Packet::makeHeader(RouteType::Direct, PayloadType::Advert, PayloadVersion::V1);
    original.pathLen = 3;
    original.path[0] = 0x01;
    original.path[1] = 0x02;
    original.path[2] = 0x03;
    original.payloadLen = 4;
    original.payload[0] = 0xDE;
    original.payload[1] = 0xAD;
    original.payload[2] = 0xBE;
    original.payload[3] = 0xEF;
        uint8_t wire[MAX_MTU_SIZE];
    uint8_t wireLen = 0;
    auto encodeStatus = encodePacket(original, wire, wireLen);
    TEST_ASSERT_TRUE(encodeStatus.isOk());
    TEST_ASSERT_EQUAL_UINT8(9, wireLen);
        Packet decoded;
    auto decodeStatus = decodePacket(wire, wireLen, decoded);
    TEST_ASSERT_TRUE(decodeStatus.isOk());
        TEST_ASSERT_EQUAL_HEX8(original.header, decoded.header);
    TEST_ASSERT_EQUAL_UINT8(original.pathLen, decoded.pathLen);
    TEST_ASSERT_EQUAL_UINT16(original.payloadLen, decoded.payloadLen);
}
void test_packet_with_transport_codes() {
    uint8_t raw[] = {
        Packet::makeHeader(RouteType::TransportDirect, PayloadType::Control),
        0x34, 0x12,
        0x78, 0x56,
        0x00,
        0xFF
    };
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_TRUE(pkt.hasTransportCodes());
    TEST_ASSERT_TRUE(pkt.isDirectRoute());
    TEST_ASSERT_EQUAL_HEX16(0x1234, pkt.transportCodes[0]);
    TEST_ASSERT_EQUAL_HEX16(0x5678, pkt.transportCodes[1]);
}
void test_packet_decode_rejects_invalid_input() {
    Packet pkt;

    auto status1 = decodePacket(nullptr, 10, pkt);
    TEST_ASSERT_TRUE(status1.isError());

    uint8_t raw[] = {0x01, 0x00};
    auto status2 = decodePacket(raw, sizeof(raw), pkt);
    TEST_ASSERT_TRUE(status2.isError());

    uint8_t raw2[] = {0x01, 0x10, 0xAA};
    auto status3 = decodePacket(raw2, sizeof(raw2), pkt);
    TEST_ASSERT_TRUE(status3.isError());
}
void test_packet_header_parsing() {
    Packet pkt;

    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    TEST_ASSERT_TRUE(pkt.isFloodRoute());
    TEST_ASSERT_FALSE(pkt.isDirectRoute());
    TEST_ASSERT_FALSE(pkt.hasTransportCodes());
        pkt.header = Packet::makeHeader(RouteType::TransportFlood, PayloadType::Ack);
    TEST_ASSERT_TRUE(pkt.isFloodRoute());
    TEST_ASSERT_TRUE(pkt.hasTransportCodes());
        pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Ack);
    TEST_ASSERT_TRUE(pkt.isDirectRoute());
    TEST_ASSERT_FALSE(pkt.hasTransportCodes());
        pkt.header = Packet::makeHeader(RouteType::TransportDirect, PayloadType::Ack);
    TEST_ASSERT_TRUE(pkt.isDirectRoute());
    TEST_ASSERT_TRUE(pkt.hasTransportCodes());
}
void test_packet_payload_types() {
    Packet pkt;
        const PayloadType types[] = {
        PayloadType::Request, PayloadType::Response, PayloadType::TextMessage,
        PayloadType::Ack, PayloadType::Advert, PayloadType::GroupText,
        PayloadType::GroupData, PayloadType::AnonRequest, PayloadType::Path,
        PayloadType::Trace, PayloadType::Multipart, PayloadType::Control
    };
        for (auto type : types) {
        pkt.header = Packet::makeHeader(RouteType::Flood, type);
        TEST_ASSERT_EQUAL(type, pkt.getPayloadType());
    }
}
