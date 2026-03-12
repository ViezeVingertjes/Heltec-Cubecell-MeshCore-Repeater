#include <unity.h>
#include "routing/repeater.h"
#include "core/config.h"
#include "core/result.h"
#include <cstring>
using namespace MiniCore;
void test_repeater_config_default_values() {
    RepeaterConfig config;
        TEST_ASSERT_TRUE(config.enabled);
    TEST_ASSERT_EQUAL_UINT8(DEFAULT_MAX_FLOOD_PATH, config.maxFloodPath);
}
void test_repeater_slot_airtime_is_300ms() {

    TEST_ASSERT_EQUAL_UINT32(300, Config::REPEATER_SLOT_AIRTIME_MS);
}
void test_repeater_disabled_returns_no_action() {
    PacketHistory history;
    RepeaterConfig config;
    config.enabled = false;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 100;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_direct_route_not_next_hop_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    pkt.pathLen = 2;
    pkt.path[0] = 0x99;
    pkt.path[1] = 0xCD;
    pkt.payloadLen = 20;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_flood_route_returns_forward() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 100;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_transport_flood_returns_forward() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::TransportFlood, PayloadType::TextMessage);
    pkt.pathLen = 1;
    pkt.payloadLen = 50;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_path_at_max_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    config.maxFloodPath = 8;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 8;
    pkt.payloadLen = 100;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_path_exceeds_max_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    config.maxFloodPath = 4;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 10;
    pkt.payloadLen = 100;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_duplicate_packet_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 50;
    std::memset(pkt.payload, 0xAB, 50);
        repeater.shouldForward(pkt);
    auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_marks_packet_as_seen() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 10;
    std::memset(pkt.payload, 0x55, 10);
        repeater.shouldForward(pkt);

    TEST_ASSERT_TRUE(history.contains(pkt));
}
void test_repeater_append_self_hash_increments_path() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        uint8_t selfHash = 0xAB;
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 2;
    pkt.path[0] = 0x11;
    pkt.path[1] = 0x22;
    pkt.payloadLen = 10;
        auto status = repeater.appendSelfHash(pkt, selfHash);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(3, pkt.pathLen);
    TEST_ASSERT_EQUAL_HEX8(0x11, pkt.path[0]);
    TEST_ASSERT_EQUAL_HEX8(0x22, pkt.path[1]);
    TEST_ASSERT_EQUAL_HEX8(0xAB, pkt.path[2]);
}
void test_repeater_append_self_hash_fails_at_max_path() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = MAX_PATH_SIZE;
    pkt.payloadLen = 10;
        auto status = repeater.appendSelfHash(pkt, 0xFF);
        TEST_ASSERT_FALSE(status.isOk());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::BufferOverflow), static_cast<int>(status.error()));
    TEST_ASSERT_EQUAL_UINT8(MAX_PATH_SIZE, pkt.pathLen);
}
void test_repeater_calculate_delay_returns_randomized_value() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 3;
    pkt.payloadLen = 100;
        uint32_t delay1 = repeater.calculateRetransmitDelay(pkt, 500);
        TEST_ASSERT_TRUE(delay1 <= 6 * 500);
}
void test_repeater_calculate_delay_trace_returns_zero() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);

    Packet tracePkt;
    tracePkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    tracePkt.pathLen = 5;
    tracePkt.payloadLen = 20;
        uint32_t delay = repeater.calculateRetransmitDelay(tracePkt, 500);

    TEST_ASSERT_EQUAL_UINT32(0, delay);
}
void test_repeater_calculate_delay_direct_non_trace_has_delay() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);

    Packet directPkt;
    directPkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    directPkt.pathLen = 3;
    directPkt.payloadLen = 20;
        uint32_t delay = repeater.calculateRetransmitDelay(directPkt, 500);

    TEST_ASSERT_EQUAL_UINT32(1500, delay);
}
void test_repeater_get_priority_based_on_payload_type() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet textMsg;
    textMsg.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    textMsg.pathLen = 1;
        uint8_t textPri = repeater.getPriority(textMsg);
        TEST_ASSERT_EQUAL_UINT8(1, textPri);
}
void test_repeater_direct_route_we_are_next_hop_returns_forward() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    pkt.pathLen = 3;
    pkt.path[0] = 0xAB;
    pkt.path[1] = 0xCD;
    pkt.path[2] = 0xEF;
    pkt.payloadLen = 5;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_direct_route_empty_path_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    pkt.pathLen = 0;
    pkt.payloadLen = 5;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_remove_self_from_path_shifts_path() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.pathLen = 3;
    pkt.path[0] = 0xAB;
    pkt.path[1] = 0xCD;
    pkt.path[2] = 0xEF;
        repeater.removeSelfFromPath(pkt);
        TEST_ASSERT_EQUAL_UINT8(2, pkt.pathLen);
    TEST_ASSERT_EQUAL_HEX8(0xCD, pkt.path[0]);
    TEST_ASSERT_EQUAL_HEX8(0xEF, pkt.path[1]);
}
void test_repeater_remove_self_from_path_single_element() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.pathLen = 1;
    pkt.path[0] = 0xAB;
        repeater.removeSelfFromPath(pkt);
        TEST_ASSERT_EQUAL_UINT8(0, pkt.pathLen);
}
void test_repeater_transport_direct_we_are_next_hop_returns_forward() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::TransportDirect, PayloadType::TextMessage);
    pkt.pathLen = 2;
    pkt.path[0] = 0xAB;
    pkt.path[1] = 0xCD;
    pkt.payloadLen = 5;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_direct_route_duplicate_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    pkt.pathLen = 2;
    pkt.path[0] = 0xAB;
    pkt.path[1] = 0xCD;
    pkt.payloadLen = 5;
    std::memset(pkt.payload, 0xDE, 5);
        repeater.shouldForward(pkt);
    auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_direct_priority_is_highest() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet directPkt;
    directPkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    directPkt.pathLen = 5;
        Packet floodPkt;
    floodPkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    floodPkt.pathLen = 1;
        uint8_t directPri = repeater.getPriority(directPkt);
    uint8_t floodPri = repeater.getPriority(floodPkt);
        TEST_ASSERT_TRUE(directPri < floodPri);
}
void test_repeater_trace_we_are_next_hop_returns_forward() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    pkt.pathLen = 0;
        uint8_t i = 0;
    uint32_t tag = 0x12345678;
    uint32_t auth = 0xDEADBEEF;
    uint8_t flags = 0x00;
    std::memcpy(&pkt.payload[i], &tag, 4); i += 4;
    std::memcpy(&pkt.payload[i], &auth, 4); i += 4;
    pkt.payload[i++] = flags;
    pkt.payload[i++] = 0xAB;
    pkt.payload[i++] = 0xCD;
    pkt.payload[i++] = 0xEF;
    pkt.payloadLen = i;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_trace_not_next_hop_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    pkt.pathLen = 0;
        uint8_t i = 0;
    uint32_t tag = 0x12345678;
    uint32_t auth = 0xDEADBEEF;
    uint8_t flags = 0x00;
    std::memcpy(&pkt.payload[i], &tag, 4); i += 4;
    std::memcpy(&pkt.payload[i], &auth, 4); i += 4;
    pkt.payload[i++] = flags;
    pkt.payload[i++] = 0x99;
    pkt.payload[i++] = 0xAB;
    pkt.payload[i++] = 0xEF;
    pkt.payloadLen = i;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_trace_second_hop_check() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xCD);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    pkt.pathLen = 1;
    pkt.path[0] = 0x50;
        uint8_t i = 0;
    uint32_t tag = 0x12345678;
    uint32_t auth = 0xDEADBEEF;
    uint8_t flags = 0x00;
    std::memcpy(&pkt.payload[i], &tag, 4); i += 4;
    std::memcpy(&pkt.payload[i], &auth, 4); i += 4;
    pkt.payload[i++] = flags;
    pkt.payload[i++] = 0xAB;
    pkt.payload[i++] = 0xCD;
    pkt.payload[i++] = 0xEF;
    pkt.payloadLen = i;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_trace_reached_end_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xEF);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    pkt.pathLen = 3;
        uint8_t i = 0;
    uint32_t tag = 0x12345678;
    uint32_t auth = 0xDEADBEEF;
    uint8_t flags = 0x00;
    std::memcpy(&pkt.payload[i], &tag, 4); i += 4;
    std::memcpy(&pkt.payload[i], &auth, 4); i += 4;
    pkt.payload[i++] = flags;
    pkt.payload[i++] = 0xAB;
    pkt.payload[i++] = 0xCD;
    pkt.payload[i++] = 0xEF;
    pkt.payloadLen = i;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_trace_append_snr_not_hash() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    pkt.pathLen = 1;
    pkt.path[0] = 0x50;
    pkt.snr = 10;
        repeater.appendSnrToPath(pkt);
        TEST_ASSERT_EQUAL_UINT8(2, pkt.pathLen);
    TEST_ASSERT_EQUAL_INT8(40, static_cast<int8_t>(pkt.path[1]));
}
void test_repeater_is_trace_packet() {
    Packet tracePkt;
    tracePkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    TEST_ASSERT_TRUE(tracePkt.getPayloadType() == PayloadType::Trace);
        Packet textPkt;
    textPkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    TEST_ASSERT_FALSE(textPkt.getPayloadType() == PayloadType::Trace);
}
void test_repeater_advert_priority_is_lowest() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet advertPkt;
    advertPkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    advertPkt.pathLen = 1;
        Packet textPkt;
    textPkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    textPkt.pathLen = 1;
        Packet pathPkt;
    pathPkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Path);
    pathPkt.pathLen = 1;
        uint8_t advertPri = repeater.getPriority(advertPkt);
    uint8_t textPri = repeater.getPriority(textPkt);
    uint8_t pathPri = repeater.getPriority(pathPkt);
        TEST_ASSERT_TRUE(advertPri > textPri);
    TEST_ASSERT_TRUE(pathPri < advertPri);
    TEST_ASSERT_TRUE(pathPri > textPri);
}
void test_repeater_control_packet_zero_hop_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x80;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_control_packet_with_path_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    pkt.pathLen = 2;
    pkt.path[0] = 0xAB;
    pkt.path[1] = 0xCD;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x80;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_direct_packet_final_destination_unsupported_drops() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::AnonRequest);
    pkt.pathLen = 1;
    pkt.path[0] = 0xAB;
    pkt.payloadLen = 10;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_direct_packet_final_destination_control_drops() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    pkt.pathLen = 1;
    pkt.path[0] = 0xAB;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x80;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_direct_packet_not_final_destination_forwards() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::AnonRequest);
    pkt.pathLen = 2;
    pkt.path[0] = 0xAB;
    pkt.path[1] = 0xCD;
    pkt.payloadLen = 10;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_control_packet_flood_route_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Control);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x80;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_is_control_packet() {
    Packet controlPkt;
    controlPkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    TEST_ASSERT_TRUE(controlPkt.getPayloadType() == PayloadType::Control);
        Packet textPkt;
    textPkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    TEST_ASSERT_FALSE(textPkt.getPayloadType() == PayloadType::Control);
}
void test_is_valid_control_request_direct_zero_hop_with_flag() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x80;
        TEST_ASSERT_TRUE(isValidControlRequest(pkt));
}
void test_is_valid_control_request_fails_flood_route() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Control);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x80;
        TEST_ASSERT_FALSE(isValidControlRequest(pkt));
}
void test_is_valid_control_request_fails_non_zero_path() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    pkt.pathLen = 1;
    pkt.path[0] = 0xAB;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x80;
        TEST_ASSERT_FALSE(isValidControlRequest(pkt));
}
void test_is_valid_control_request_fails_missing_flag() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x00;
        TEST_ASSERT_FALSE(isValidControlRequest(pkt));
}
void test_is_valid_control_request_fails_empty_payload() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    pkt.pathLen = 0;
    pkt.payloadLen = 0;
        TEST_ASSERT_FALSE(isValidControlRequest(pkt));
}
void test_is_valid_control_request_fails_non_control_type() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x80;
        TEST_ASSERT_FALSE(isValidControlRequest(pkt));
}

void test_repeater_trace_2byte_hash_we_are_next_hop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);

    uint8_t selfHash[8] = {0xAB, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    repeater.setSelfHashMultiByte(selfHash, 8);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    pkt.pathLen = 0;
        uint8_t i = 0;
    uint32_t tag = 0x12345678;
    uint32_t auth = 0xDEADBEEF;
    uint8_t flags = 0x01;
    std::memcpy(&pkt.payload[i], &tag, 4); i += 4;
    std::memcpy(&pkt.payload[i], &auth, 4); i += 4;
    pkt.payload[i++] = flags;

    pkt.payload[i++] = 0xAB;
    pkt.payload[i++] = 0xCD;
    pkt.payload[i++] = 0x11;
    pkt.payload[i++] = 0x22;
    pkt.payloadLen = i;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_trace_2byte_hash_partial_match_drops() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);

    uint8_t selfHash[8] = {0xAB, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    repeater.setSelfHashMultiByte(selfHash, 8);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    pkt.pathLen = 0;
        uint8_t i = 0;
    uint32_t tag = 0x12345678;
    uint32_t auth = 0xDEADBEEF;
    uint8_t flags = 0x01;
    std::memcpy(&pkt.payload[i], &tag, 4); i += 4;
    std::memcpy(&pkt.payload[i], &auth, 4); i += 4;
    pkt.payload[i++] = flags;

    pkt.payload[i++] = 0xAB;
    pkt.payload[i++] = 0xFF;
    pkt.payload[i++] = 0x11;
    pkt.payload[i++] = 0x22;
    pkt.payloadLen = i;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_trace_4byte_hash_we_are_next_hop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
        uint8_t selfHash[8] = {0xAB, 0xCD, 0xEF, 0x12, 0x00, 0x00, 0x00, 0x00};
    repeater.setSelfHashMultiByte(selfHash, 8);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    pkt.pathLen = 0;
        uint8_t i = 0;
    uint32_t tag = 0x12345678;
    uint32_t auth = 0xDEADBEEF;
    uint8_t flags = 0x02;
    std::memcpy(&pkt.payload[i], &tag, 4); i += 4;
    std::memcpy(&pkt.payload[i], &auth, 4); i += 4;
    pkt.payload[i++] = flags;

    pkt.payload[i++] = 0xAB;
    pkt.payload[i++] = 0xCD;
    pkt.payload[i++] = 0xEF;
    pkt.payload[i++] = 0x12;
    pkt.payloadLen = i;
        auto action = repeater.shouldForward(pkt);
        TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
