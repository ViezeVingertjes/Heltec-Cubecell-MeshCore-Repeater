#include <unity.h>
#include <cstring>
#include "core/config.h"
#include "routing/packet_router.h"
#include "helpers/packet_router_events_helper.h"
using namespace MiniCore;

void test_packet_router_duplicate_flood_never_forwarded_twice() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true, .maxFloodPath = 64};
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 1;
    pkt.path[0] = 0xBB;
    pkt.payloadLen = 5;
    pkt.payload[0] = 0xCC;
    std::memset(&pkt.payload[1], 0x42, 4);
    RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    int forwardCount = 0;
    for (int i = 0; i < 10; ++i) {
        events.reset();
        Packet pktCopy = pkt;
        router.processPacket(pktCopy, rxInfo);
        if (events.forwardPacketCalled) {
            ++forwardCount;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, forwardCount);
}
void test_packet_router_duplicate_control_request_never_triggers_twice() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true};
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    pkt.pathLen = 0;
    pkt.payloadLen = 6;
    pkt.payload[0] = Config::CTL_TYPE_NODE_DISCOVER_REQ;
    pkt.payload[1] = Config::ADV_TYPE_REPEATER;
    pkt.payload[2] = 0x01;
    pkt.payload[3] = 0x02;
    pkt.payload[4] = 0x03;
    pkt.payload[5] = 0x04;
    RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    int discoveryCount = 0;
    for (int i = 0; i < 5; ++i) {
        events.reset();
        Packet pktCopy = pkt;
        router.processPacket(pktCopy, rxInfo);
        if (events.discoveryRequestCalled) {
            ++discoveryCount;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, discoveryCount);
}
void test_packet_router_duplicate_advert_still_processes_for_time_sync() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true, .maxFloodPath = 64};
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 100;
    pkt.payload[0] = 0x42;
    for (int i = 1; i < 32; ++i) pkt.payload[i] = 0x00;
    pkt.payload[32] = 0x78;
    pkt.payload[33] = 0x56;
    pkt.payload[34] = 0x34;
    pkt.payload[35] = 0x12;
    for (int i = 36; i < 100; ++i) pkt.payload[i] = 0x00;
    RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    int advertCount = 0;
    int forwardCount = 0;
    for (int i = 0; i < 3; ++i) {
        events.reset();
        Packet pktCopy = pkt;
        router.processPacket(pktCopy, rxInfo);
        if (events.advertReceivedCalled) {
            ++advertCount;
        }
        if (events.forwardPacketCalled) {
            ++forwardCount;
        }
    }
    TEST_ASSERT_EQUAL_INT(3, advertCount);
    TEST_ASSERT_EQUAL_INT(1, forwardCount);
}
void test_packet_router_different_packets_both_forwarded() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true, .maxFloodPath = 64};
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    Packet pkt1;
    pkt1.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt1.pathLen = 0;
    pkt1.payloadLen = 5;
    pkt1.payload[0] = 0xCC;
    std::memset(&pkt1.payload[1], 0x11, 4);
    Packet pkt2;
    pkt2.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt2.pathLen = 0;
    pkt2.payloadLen = 5;
    pkt2.payload[0] = 0xCC;
    std::memset(&pkt2.payload[1], 0x22, 4);
    RxPacket rxInfo{nullptr, 0, -50, 10};
    events.reset();
    router.processPacket(pkt1, rxInfo);
    TEST_ASSERT_TRUE(events.forwardPacketCalled);
    events.reset();
    router.processPacket(pkt2, rxInfo);
    TEST_ASSERT_TRUE(events.forwardPacketCalled);
}

void test_packet_router_control_response_marked_seen_but_not_processed() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true};
    Repeater repeater{history, config};
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    pkt.pathLen = 0;
    pkt.payloadLen = 10;
    pkt.payload[0] = Config::CTL_TYPE_NODE_DISCOVER_RESP;
    std::memset(&pkt.payload[1], 0x00, 9);
    RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    router.processPacket(pkt, rxInfo);
    TEST_ASSERT_FALSE(events.discoveryRequestCalled);
    TEST_ASSERT_FALSE(events.forwardPacketCalled);
    events.reset();
    router.processPacket(pkt, rxInfo);
    TEST_ASSERT_FALSE(events.discoveryRequestCalled);
}
void test_packet_router_null_event_handler_no_crash() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true, .maxFloodPath = 64};
    Repeater repeater{history, config};
    PacketRouter router{repeater};
    router.setSelfHash(0xAA);
    Packet advertPkt;
    advertPkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    advertPkt.pathLen = 0;
    advertPkt.payloadLen = 100;
    advertPkt.payload[0] = 0x42;
    for (int i = 1; i < 32; ++i) advertPkt.payload[i] = 0x00;
    advertPkt.payload[32] = 0x78;
    advertPkt.payload[33] = 0x56;
    advertPkt.payload[34] = 0x34;
    advertPkt.payload[35] = 0x12;
    for (int i = 36; i < 100; ++i) advertPkt.payload[i] = 0x00;
    Packet ctrlPkt;
    ctrlPkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    ctrlPkt.pathLen = 0;
    ctrlPkt.payloadLen = 6;
    ctrlPkt.payload[0] = Config::CTL_TYPE_NODE_DISCOVER_REQ;
    ctrlPkt.payload[1] = Config::ADV_TYPE_REPEATER;
    std::memset(&ctrlPkt.payload[2], 0x00, 4);
    Packet floodPkt;
    floodPkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    floodPkt.pathLen = 0;
    floodPkt.payloadLen = 5;
    floodPkt.payload[0] = 0xCC;
    std::memset(&floodPkt.payload[1], 0x11, 4);
    RxPacket rxInfo{nullptr, 0, -50, 10};
    router.processPacket(advertPkt, rxInfo);
    router.processPacket(ctrlPkt, rxInfo);
    router.processPacket(floodPkt, rxInfo);
    TEST_PASS();
}
void test_packet_router_packet_exceeds_path_limit_not_forwarded() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true, .maxFloodPath = 3};
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 5;
    pkt.path[0] = 0x11;
    pkt.path[1] = 0x22;
    pkt.path[2] = 0x33;
    pkt.path[3] = 0x44;
    pkt.path[4] = 0x55;
    pkt.payloadLen = 5;
    pkt.payload[0] = 0xCC;
    std::memset(&pkt.payload[1], 0x11, 4);
    RxPacket rxInfo{nullptr, 0, -50, 10};
    router.processPacket(pkt, rxInfo);
    TEST_ASSERT_FALSE(events.forwardPacketCalled);
}
void test_packet_router_repeater_disabled_no_forward() {
    PacketHistory history;
    RepeaterConfig config{.enabled = false};
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 0;
    pkt.payloadLen = 5;
    pkt.payload[0] = 0xCC;
    std::memset(&pkt.payload[1], 0x11, 4);
    RxPacket rxInfo{nullptr, 0, -50, 10};
    router.processPacket(pkt, rxInfo);
    TEST_ASSERT_FALSE(events.forwardPacketCalled);
}
void test_packet_router_malformed_discover_request_still_deduped() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true};
    Repeater repeater{history, config};
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
    pkt.pathLen = 0;
    pkt.payloadLen = 2;
    pkt.payload[0] = Config::CTL_TYPE_NODE_DISCOVER_REQ;
    pkt.payload[1] = 0xFF;
    RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    router.processPacket(pkt, rxInfo);
    TEST_ASSERT_FALSE(events.discoveryRequestCalled);
    events.reset();
    router.processPacket(pkt, rxInfo);
    TEST_ASSERT_FALSE(events.discoveryRequestCalled);
}
void test_packet_router_flood_destined_for_self_not_forwarded() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true, .maxFloodPath = 64};
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 0;
    pkt.payloadLen = 5;
    pkt.payload[0] = 0xAA;
    std::memset(&pkt.payload[1], 0x11, 4);
    RxPacket rxInfo{nullptr, 0, -50, 10};
    router.processPacket(pkt, rxInfo);
    TEST_ASSERT_FALSE(events.forwardPacketCalled);
}
void test_packet_router_stress_many_unique_packets() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true, .maxFloodPath = 64};
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    RxPacket rxInfo{nullptr, 0, -50, 10};
    int forwardCount = 0;
    for (int i = 0; i < 100; ++i) {
        events.reset();
        Packet pkt;
        pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
        pkt.pathLen = 0;
        pkt.payloadLen = 5;
        pkt.payload[0] = 0xCC;
        pkt.payload[1] = static_cast<uint8_t>(i);
        pkt.payload[2] = static_cast<uint8_t>(i >> 8);
        pkt.payload[3] = 0xBB;
        pkt.payload[4] = 0xDD;
        router.processPacket(pkt, rxInfo);
        if (events.forwardPacketCalled) {
            ++forwardCount;
        }
    }
    TEST_ASSERT_EQUAL_INT(100, forwardCount);
}
void test_packet_router_stress_same_packet_many_times() {
    PacketHistory history;
    RepeaterConfig config{.enabled = true, .maxFloodPath = 64};
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 0;
    pkt.payloadLen = 5;
    pkt.payload[0] = 0xCC;
    std::memset(&pkt.payload[1], 0x42, 4);
    RxPacket rxInfo{nullptr, 0, -50, 10};
    int forwardCount = 0;
    for (int i = 0; i < 100; ++i) {
        events.reset();
        Packet pktCopy = pkt;
        router.processPacket(pktCopy, rxInfo);
        if (events.forwardPacketCalled) {
            ++forwardCount;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, forwardCount);
}
