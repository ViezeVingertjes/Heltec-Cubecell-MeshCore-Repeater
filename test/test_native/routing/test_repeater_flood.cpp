#include <unity.h>
#include "routing/repeater.h"
#include "core/config.h"
#include "core/result.h"
#include <cstring>
using namespace MiniCore;

void test_repeater_flood_packet_with_self_in_path_still_forwards() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::AnonRequest);
    pkt.pathLen = 1;
    pkt.path[0] = 0xAB;
    pkt.payloadLen = 10;
    std::memset(pkt.payload, 0x55, 10);
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_flood_packet_final_destination_control_drops() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Control);
    pkt.pathLen = 1;
    pkt.path[0] = 0xAB;
    pkt.payloadLen = 1;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_flood_packet_not_final_destination_forwards() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::AnonRequest);
    pkt.pathLen = 1;
    pkt.path[0] = 0xCD;
    pkt.payloadLen = 10;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_raw_custom_flood_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::RawCustom);
    pkt.pathLen = 0;
    pkt.payloadLen = 10;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_raw_custom_transport_flood_returns_drop() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::TransportFlood, PayloadType::RawCustom);
    pkt.pathLen = 0;
    pkt.payloadLen = 10;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_raw_custom_direct_we_are_next_hop_forwards() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::RawCustom);
    pkt.pathLen = 2;
    pkt.path[0] = 0xAB;
    pkt.path[1] = 0xCD;
    pkt.payloadLen = 10;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_flood_with_self_hash_in_path_still_forwards_if_not_seen() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 1;
    pkt.path[0] = 0xAB;
    pkt.payloadLen = 20;
    pkt.payload[0] = 0xCD;
    std::memset(&pkt.payload[1], 0x55, 19);
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_flood_anon_request_destined_for_self_drops() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::AnonRequest);
    pkt.pathLen = 0;
    pkt.payloadLen = 40;
    pkt.payload[0] = 0xAB;
    std::memset(&pkt.payload[1], 0x55, 39);
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_flood_anon_request_destined_for_other_forwards() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::AnonRequest);
    pkt.pathLen = 0;
    pkt.payloadLen = 40;
    pkt.payload[0] = 0xCD;
    std::memset(&pkt.payload[1], 0x55, 39);
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_flood_request_destined_for_self_drops() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Request);
    pkt.pathLen = 0;
    pkt.payloadLen = 20;
    pkt.payload[0] = 0xAB;
    std::memset(&pkt.payload[1], 0x55, 19);
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_flood_response_destined_for_self_drops() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Response);
    pkt.pathLen = 0;
    pkt.payloadLen = 20;
    pkt.payload[0] = 0xAB;
    std::memset(&pkt.payload[1], 0x55, 19);
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_flood_text_message_destined_for_self_drops() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 0;
    pkt.payloadLen = 20;
    pkt.payload[0] = 0xAB;
    std::memset(&pkt.payload[1], 0x55, 19);
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_flood_path_destined_for_self_drops() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Path);
    pkt.pathLen = 0;
    pkt.payloadLen = 20;
    pkt.payload[0] = 0xAB;
    std::memset(&pkt.payload[1], 0x55, 19);
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_flood_advert_no_dest_hash_forwards() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 100;
    pkt.payload[0] = 0xAB;
    std::memset(&pkt.payload[1], 0x55, 99);
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_flood_ack_no_dest_hash_forwards() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater(history, config);
    repeater.setSelfHash(0xAB);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    pkt.pathLen = 0;
    pkt.payloadLen = 4;
    pkt.payload[0] = 0xAB;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_config_drop_repeater_adverts_default_false() {
    RepeaterConfig config;
    TEST_ASSERT_FALSE(config.dropRepeaterAdverts);
}
void test_repeater_drop_repeater_adverts_disabled_forwards_all() {
    PacketHistory history;
    RepeaterConfig config;
    config.dropRepeaterAdverts = false;
    Repeater repeater(history, config);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 101;
    std::memset(pkt.payload, 0x55, 100);
    pkt.payload[100] = Config::ADV_TYPE_REPEATER;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_drop_repeater_adverts_enabled_drops_repeater_advert() {
    PacketHistory history;
    RepeaterConfig config;
    config.dropRepeaterAdverts = true;
    Repeater repeater(history, config);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 101;
    std::memset(pkt.payload, 0x55, 100);
    pkt.payload[100] = Config::ADV_TYPE_REPEATER;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_drop_repeater_adverts_enabled_forwards_companion_advert() {
    PacketHistory history;
    RepeaterConfig config;
    config.dropRepeaterAdverts = true;
    Repeater repeater(history, config);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 101;
    std::memset(pkt.payload, 0x55, 100);
    pkt.payload[100] = Config::ADV_TYPE_CHAT_CLIENT;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_drop_repeater_adverts_enabled_drops_room_advert() {
    PacketHistory history;
    RepeaterConfig config;
    config.dropRepeaterAdverts = true;
    Repeater repeater(history, config);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 101;
    std::memset(pkt.payload, 0x55, 100);
    pkt.payload[100] = Config::ADV_TYPE_ROOM;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Drop, action);
}
void test_repeater_drop_repeater_adverts_enabled_still_forwards_text() {
    PacketHistory history;
    RepeaterConfig config;
    config.dropRepeaterAdverts = true;
    Repeater repeater(history, config);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 0;
    pkt.payloadLen = 20;
    pkt.payload[0] = 0xCD;
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
void test_repeater_drop_repeater_adverts_short_payload_forwards() {
    PacketHistory history;
    RepeaterConfig config;
    config.dropRepeaterAdverts = true;
    Repeater repeater(history, config);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 100;
    std::memset(pkt.payload, 0x55, 100);
    auto action = repeater.shouldForward(pkt);
    TEST_ASSERT_EQUAL(RepeaterAction::Forward, action);
}
