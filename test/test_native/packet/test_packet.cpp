#include <unity.h>
#include "packet/packet.h"
#include "routing/repeater.h"
#include <cstring>
using namespace MiniCore;

void test_packet_default_construction() {
    Packet pkt;
    TEST_ASSERT_EQUAL_UINT8(0, pkt.header);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.pathLen);
    TEST_ASSERT_EQUAL_UINT16(0, pkt.payloadLen);
    TEST_ASSERT_EQUAL_UINT16(0, pkt.transportCodes[0]);
    TEST_ASSERT_EQUAL_UINT16(0, pkt.transportCodes[1]);
    TEST_ASSERT_EQUAL_INT8(0, pkt.snr);
}
void test_packet_make_header() {

    uint8_t header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage, PayloadVersion::V1);
    TEST_ASSERT_EQUAL_HEX8(0x09, header);
}
void test_packet_make_header_with_version() {

    uint8_t header = Packet::makeHeader(RouteType::Direct, PayloadType::Advert, PayloadVersion::V2);
    TEST_ASSERT_EQUAL_HEX8(0x52, header);
}
void test_packet_get_route_type_flood() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    TEST_ASSERT_EQUAL(RouteType::Flood, pkt.getRouteType());
}
void test_packet_get_route_type_direct() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Ack);
    TEST_ASSERT_EQUAL(RouteType::Direct, pkt.getRouteType());
}
void test_packet_get_route_type_transport_flood() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::TransportFlood, PayloadType::Ack);
    TEST_ASSERT_EQUAL(RouteType::TransportFlood, pkt.getRouteType());
}
void test_packet_get_route_type_transport_direct() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::TransportDirect, PayloadType::Ack);
    TEST_ASSERT_EQUAL(RouteType::TransportDirect, pkt.getRouteType());
}
void test_packet_get_payload_type() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    TEST_ASSERT_EQUAL(PayloadType::TextMessage, pkt.getPayloadType());
}
void test_packet_get_payload_type_all_values() {
    Packet pkt;
        const PayloadType types[] = {
        PayloadType::Request, PayloadType::Response, PayloadType::TextMessage,
        PayloadType::Ack, PayloadType::Advert, PayloadType::GroupText,
        PayloadType::GroupData, PayloadType::AnonRequest, PayloadType::Path,
        PayloadType::Trace, PayloadType::Multipart, PayloadType::Control,
        PayloadType::RawCustom
    };
        for (auto type : types) {
        pkt.header = Packet::makeHeader(RouteType::Flood, type);
        TEST_ASSERT_EQUAL(type, pkt.getPayloadType());
    }
}
void test_packet_get_payload_version() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack, PayloadVersion::V3);
    TEST_ASSERT_EQUAL(PayloadVersion::V3, pkt.getPayloadVersion());
}
void test_packet_has_transport_codes_false() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    TEST_ASSERT_FALSE(pkt.hasTransportCodes());
        pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Ack);
    TEST_ASSERT_FALSE(pkt.hasTransportCodes());
}
void test_packet_has_transport_codes_true() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::TransportFlood, PayloadType::Ack);
    TEST_ASSERT_TRUE(pkt.hasTransportCodes());
        pkt.header = Packet::makeHeader(RouteType::TransportDirect, PayloadType::Ack);
    TEST_ASSERT_TRUE(pkt.hasTransportCodes());
}
void test_packet_is_flood_route() {
    Packet pkt;
        pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    TEST_ASSERT_TRUE(pkt.isFloodRoute());
        pkt.header = Packet::makeHeader(RouteType::TransportFlood, PayloadType::Ack);
    TEST_ASSERT_TRUE(pkt.isFloodRoute());
        pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Ack);
    TEST_ASSERT_FALSE(pkt.isFloodRoute());
}
void test_packet_is_direct_route() {
    Packet pkt;
        pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Ack);
    TEST_ASSERT_TRUE(pkt.isDirectRoute());
        pkt.header = Packet::makeHeader(RouteType::TransportDirect, PayloadType::Ack);
    TEST_ASSERT_TRUE(pkt.isDirectRoute());
        pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    TEST_ASSERT_FALSE(pkt.isDirectRoute());
}

void test_decode_minimal_packet() {

    uint8_t raw[] = {
        Packet::makeHeader(RouteType::Flood, PayloadType::Ack),
        0x00,
        0xAB
    };
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL(RouteType::Flood, pkt.getRouteType());
    TEST_ASSERT_EQUAL(PayloadType::Ack, pkt.getPayloadType());
    TEST_ASSERT_EQUAL_UINT8(0, pkt.pathLen);
    TEST_ASSERT_EQUAL_UINT16(1, pkt.payloadLen);
    TEST_ASSERT_EQUAL_HEX8(0xAB, pkt.payload[0]);
}
void test_decode_packet_with_path() {

    uint8_t raw[] = {
        Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage),
        0x03,
        0x11, 0x22, 0x33,
        0xAA, 0xBB
    };
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(3, pkt.pathLen);
    TEST_ASSERT_EQUAL_HEX8(0x11, pkt.path[0]);
    TEST_ASSERT_EQUAL_HEX8(0x22, pkt.path[1]);
    TEST_ASSERT_EQUAL_HEX8(0x33, pkt.path[2]);
    TEST_ASSERT_EQUAL_UINT16(2, pkt.payloadLen);
    TEST_ASSERT_EQUAL_HEX8(0xAA, pkt.payload[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, pkt.payload[1]);
}
void test_decode_packet_with_transport_codes() {

    uint8_t raw[] = {
        Packet::makeHeader(RouteType::TransportFlood, PayloadType::Request),
        0x34, 0x12,
        0x78, 0x56,
        0x00,
        0xCC
    };
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_TRUE(pkt.hasTransportCodes());
    TEST_ASSERT_EQUAL_HEX16(0x1234, pkt.transportCodes[0]);
    TEST_ASSERT_EQUAL_HEX16(0x5678, pkt.transportCodes[1]);
    TEST_ASSERT_EQUAL_UINT16(1, pkt.payloadLen);
    TEST_ASSERT_EQUAL_HEX8(0xCC, pkt.payload[0]);
}
void test_decode_packet_direct_route_with_path() {

    uint8_t path[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t raw[2 + sizeof(path) + 4];
    raw[0] = Packet::makeHeader(RouteType::Direct, PayloadType::Response);
    raw[1] = sizeof(path);
    std::memcpy(&raw[2], path, sizeof(path));
    raw[2 + sizeof(path)] = 0xDE;
    raw[3 + sizeof(path)] = 0xAD;
    raw[4 + sizeof(path)] = 0xBE;
    raw[5 + sizeof(path)] = 0xEF;
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_TRUE(pkt.isDirectRoute());
    TEST_ASSERT_EQUAL_UINT8(5, pkt.pathLen);
    TEST_ASSERT_EQUAL_UINT16(4, pkt.payloadLen);
    TEST_ASSERT_EQUAL_MEMORY(path, pkt.path, sizeof(path));
}
void test_decode_fails_empty_buffer() {
    Packet pkt;
    auto status = decodePacket(nullptr, 0, pkt);
        TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, status.error());
}
void test_decode_fails_buffer_too_small() {
    uint8_t raw[] = {0x01, 0x00};
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, status.error());
}
void test_decode_fails_path_too_long() {

    uint8_t raw[] = {
        Packet::makeHeader(RouteType::Flood, PayloadType::Ack),
        0xFF,
        0xAB
    };
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::TooManyHops, status.error());
}
void test_decode_fails_path_exceeds_buffer() {
    uint8_t raw[] = {
        Packet::makeHeader(RouteType::Flood, PayloadType::Ack),
        0x10,
        0x01
    };
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, status.error());
}
void test_decode_fails_no_payload() {

    uint8_t raw[] = {
        Packet::makeHeader(RouteType::Flood, PayloadType::Ack),
        0x01,
        0xAA
    };
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, status.error());
}
void test_decode_fails_transport_codes_truncated() {

    uint8_t raw[] = {
        Packet::makeHeader(RouteType::TransportFlood, PayloadType::Ack),
        0x12, 0x34
    };
        Packet pkt;
    auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, status.error());
}
void test_decode_clears_transport_codes_when_not_present() {
    uint8_t raw[] = {
        Packet::makeHeader(RouteType::Flood, PayloadType::Ack),
        0x00,
        0xAB
    };
        Packet pkt;
    pkt.transportCodes[0] = 0xFFFF;
    pkt.transportCodes[1] = 0xFFFF;
        auto status = decodePacket(raw, sizeof(raw), pkt);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT16(0, pkt.transportCodes[0]);
    TEST_ASSERT_EQUAL_UINT16(0, pkt.transportCodes[1]);
}

void test_encode_minimal_packet() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0xAB;
        uint8_t dest[MAX_MTU_SIZE];
    uint8_t len = 0;
    auto status = encodePacket(pkt, dest, len);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(3, len);
    TEST_ASSERT_EQUAL_HEX8(pkt.header, dest[0]);
    TEST_ASSERT_EQUAL_HEX8(0, dest[1]);
    TEST_ASSERT_EQUAL_HEX8(0xAB, dest[2]);
}
void test_encode_packet_with_path() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    pkt.pathLen = 3;
    pkt.path[0] = 0x11;
    pkt.path[1] = 0x22;
    pkt.path[2] = 0x33;
    pkt.payloadLen = 2;
    pkt.payload[0] = 0xAA;
    pkt.payload[1] = 0xBB;
        uint8_t dest[MAX_MTU_SIZE];
    uint8_t len = 0;
    auto status = encodePacket(pkt, dest, len);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(7, len);
    TEST_ASSERT_EQUAL_UINT8(3, dest[1]);
    TEST_ASSERT_EQUAL_HEX8(0x11, dest[2]);
    TEST_ASSERT_EQUAL_HEX8(0x22, dest[3]);
    TEST_ASSERT_EQUAL_HEX8(0x33, dest[4]);
    TEST_ASSERT_EQUAL_HEX8(0xAA, dest[5]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, dest[6]);
}
void test_encode_packet_with_transport_codes() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::TransportDirect, PayloadType::Control);
    pkt.transportCodes[0] = 0x1234;
    pkt.transportCodes[1] = 0x5678;
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0xDD;
        uint8_t dest[MAX_MTU_SIZE];
    uint8_t len = 0;
    auto status = encodePacket(pkt, dest, len);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(7, len);
    TEST_ASSERT_EQUAL_HEX8(0x34, dest[1]);
    TEST_ASSERT_EQUAL_HEX8(0x12, dest[2]);
    TEST_ASSERT_EQUAL_HEX8(0x78, dest[3]);
    TEST_ASSERT_EQUAL_HEX8(0x56, dest[4]);
    TEST_ASSERT_EQUAL_UINT8(0, dest[5]);
    TEST_ASSERT_EQUAL_HEX8(0xDD, dest[6]);
}
void test_encode_decode_roundtrip() {
    Packet original;
    original.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage, PayloadVersion::V1);
    original.pathLen = 4;
    original.path[0] = 0xA1;
    original.path[1] = 0xB2;
    original.path[2] = 0xC3;
    original.path[3] = 0xD4;
    original.payloadLen = 5;
    std::memcpy(original.payload, "Hello", 5);
        uint8_t wire[MAX_MTU_SIZE];
    uint8_t wireLen = 0;
    auto encodeStatus = encodePacket(original, wire, wireLen);
    TEST_ASSERT_TRUE(encodeStatus.isOk());
        Packet decoded;
    auto decodeStatus = decodePacket(wire, wireLen, decoded);
    TEST_ASSERT_TRUE(decodeStatus.isOk());
        TEST_ASSERT_EQUAL_HEX8(original.header, decoded.header);
    TEST_ASSERT_EQUAL_UINT8(original.pathLen, decoded.pathLen);
    TEST_ASSERT_EQUAL_MEMORY(original.path, decoded.path, original.pathLen);
    TEST_ASSERT_EQUAL_UINT16(original.payloadLen, decoded.payloadLen);
    TEST_ASSERT_EQUAL_MEMORY(original.payload, decoded.payload, original.payloadLen);
}
void test_encode_decode_roundtrip_with_transport() {
    Packet original;
    original.header = Packet::makeHeader(RouteType::TransportFlood, PayloadType::GroupData);
    original.transportCodes[0] = 0xABCD;
    original.transportCodes[1] = 0x1234;
    original.pathLen = 2;
    original.path[0] = 0xFF;
    original.path[1] = 0xEE;
    original.payloadLen = 3;
    original.payload[0] = 0x01;
    original.payload[1] = 0x02;
    original.payload[2] = 0x03;
        uint8_t wire[MAX_MTU_SIZE];
    uint8_t wireLen = 0;
    auto encodeStatus = encodePacket(original, wire, wireLen);
    TEST_ASSERT_TRUE(encodeStatus.isOk());
        Packet decoded;
    auto decodeStatus = decodePacket(wire, wireLen, decoded);
    TEST_ASSERT_TRUE(decodeStatus.isOk());
        TEST_ASSERT_EQUAL_HEX16(original.transportCodes[0], decoded.transportCodes[0]);
    TEST_ASSERT_EQUAL_HEX16(original.transportCodes[1], decoded.transportCodes[1]);
    TEST_ASSERT_EQUAL_UINT8(original.pathLen, decoded.pathLen);
    TEST_ASSERT_EQUAL_UINT16(original.payloadLen, decoded.payloadLen);
}
void test_packet_get_raw_length_minimal() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;

    TEST_ASSERT_EQUAL_UINT16(3, pkt.getRawLength());
}
void test_packet_get_raw_length_with_path() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    pkt.pathLen = 10;
    pkt.payloadLen = 20;

    TEST_ASSERT_EQUAL_UINT16(32, pkt.getRawLength());
}
void test_packet_get_raw_length_with_transport() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::TransportDirect, PayloadType::Ack);
    pkt.pathLen = 5;
    pkt.payloadLen = 10;

    TEST_ASSERT_EQUAL_UINT16(21, pkt.getRawLength());
}

void test_payload_type_name_returns_correct_strings() {
    TEST_ASSERT_EQUAL_STRING("REQ", payloadTypeName(PayloadType::Request));
    TEST_ASSERT_EQUAL_STRING("RSP", payloadTypeName(PayloadType::Response));
    TEST_ASSERT_EQUAL_STRING("TXT", payloadTypeName(PayloadType::TextMessage));
    TEST_ASSERT_EQUAL_STRING("ACK", payloadTypeName(PayloadType::Ack));
    TEST_ASSERT_EQUAL_STRING("ADV", payloadTypeName(PayloadType::Advert));
    TEST_ASSERT_EQUAL_STRING("GRP", payloadTypeName(PayloadType::GroupText));
    TEST_ASSERT_EQUAL_STRING("GRD", payloadTypeName(PayloadType::GroupData));
    TEST_ASSERT_EQUAL_STRING("ANO", payloadTypeName(PayloadType::AnonRequest));
    TEST_ASSERT_EQUAL_STRING("PTH", payloadTypeName(PayloadType::Path));
    TEST_ASSERT_EQUAL_STRING("TRC", payloadTypeName(PayloadType::Trace));
    TEST_ASSERT_EQUAL_STRING("MUL", payloadTypeName(PayloadType::Multipart));
    TEST_ASSERT_EQUAL_STRING("CTL", payloadTypeName(PayloadType::Control));
    TEST_ASSERT_EQUAL_STRING("RAW", payloadTypeName(PayloadType::RawCustom));
}
void test_route_type_name_returns_correct_strings() {
    TEST_ASSERT_EQUAL_STRING("FLD", routeTypeName(RouteType::Flood));
    TEST_ASSERT_EQUAL_STRING("DIR", routeTypeName(RouteType::Direct));
    TEST_ASSERT_EQUAL_STRING("TFL", routeTypeName(RouteType::TransportFlood));
    TEST_ASSERT_EQUAL_STRING("TDR", routeTypeName(RouteType::TransportDirect));
}
void test_format_packet_stats_minimal() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    pkt.pathLen = 0;
    pkt.payloadLen = 4;
        char buffer[64];
    formatPacketStats(pkt, buffer, sizeof(buffer));

    TEST_ASSERT_NOT_NULL(strstr(buffer, "FLD"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "ACK"));
}
void test_format_packet_stats_with_path() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    pkt.pathLen = 5;
    pkt.payloadLen = 20;
        char buffer[64];
    formatPacketStats(pkt, buffer, sizeof(buffer));
        TEST_ASSERT_NOT_NULL(strstr(buffer, "DIR"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "TXT"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "p:5"));
}
void test_format_packet_stats_buffer_safety() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 100;
        char smallBuffer[8];
    formatPacketStats(pkt, smallBuffer, sizeof(smallBuffer));

    TEST_ASSERT_EQUAL_CHAR('\0', smallBuffer[7]);
}
