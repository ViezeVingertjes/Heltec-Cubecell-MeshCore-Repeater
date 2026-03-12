#include <unity.h>
#include <cstring>
#include "routing/packet_router.h"
#include "helpers/packet_router_events_helper.h"
using namespace MiniCore;
void test_packet_router_initial_state() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater{history, config};
    PacketRouter router{repeater};
        TEST_ASSERT_EQUAL_UINT8(0, router.selfHash());
}
void test_packet_router_set_self_hash() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater{history, config};
    PacketRouter router{repeater};
        router.setSelfHash(0xAB);
        TEST_ASSERT_EQUAL_UINT8(0xAB, router.selfHash());
}
void test_packet_router_processes_advert_packet() {
    PacketHistory history;
    RepeaterConfig config;
    Repeater repeater{history, config};
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
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
    router.processPacket(pkt, rxInfo);
        TEST_ASSERT_TRUE(events.advertReceivedCalled);
    TEST_ASSERT_EQUAL_UINT8(0x42, events.lastAdvertSenderHash);
    TEST_ASSERT_EQUAL_UINT32(0x12345678, events.lastAdvertTimestamp);
}
void test_packet_router_forwards_flood_packet() {
    PacketHistory history;
    RepeaterConfig config;
    config.enabled = true;
    config.maxFloodPath = 64;
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
    pkt.payload[1] = 0x01;
    pkt.payload[2] = 0x02;
    pkt.payload[3] = 0x03;
    pkt.payload[4] = 0x04;
        RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    router.processPacket(pkt, rxInfo);
        TEST_ASSERT_TRUE(events.forwardPacketCalled);
}
void test_packet_router_does_not_forward_duplicate() {
    PacketHistory history;
    RepeaterConfig config;
    config.enabled = true;
    config.maxFloodPath = 64;
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
    pkt.payload[1] = 0x01;
    pkt.payload[2] = 0x02;
    pkt.payload[3] = 0x03;
    pkt.payload[4] = 0x04;
        RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
        router.processPacket(pkt, rxInfo);
    TEST_ASSERT_TRUE(events.forwardPacketCalled);
    TEST_ASSERT_FALSE(events.duplicateDroppedCalled);
        events.reset();

    router.processPacket(pkt, rxInfo);
    TEST_ASSERT_FALSE(events.forwardPacketCalled);
    TEST_ASSERT_TRUE(events.duplicateDroppedCalled);
}
void test_packet_router_appends_self_hash_for_flood() {
    PacketHistory history;
    RepeaterConfig config;
    config.enabled = true;
    config.maxFloodPath = 64;
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
    pkt.payload[1] = 0x01;
    pkt.payload[2] = 0x02;
    pkt.payload[3] = 0x03;
    pkt.payload[4] = 0x04;
        RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    router.processPacket(pkt, rxInfo);
        TEST_ASSERT_TRUE(events.forwardPacketCalled);
    TEST_ASSERT_EQUAL_UINT8(2, events.lastForwardPacket.pathLen);
    TEST_ASSERT_EQUAL_UINT8(0xBB, events.lastForwardPacket.path[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, events.lastForwardPacket.path[1]);
}
void test_packet_router_does_not_forward_when_disabled() {
    PacketHistory history;
    RepeaterConfig config;
    config.enabled = false;
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
    pkt.payload[1] = 0x01;
    pkt.payload[2] = 0x02;
    pkt.payload[3] = 0x03;
    pkt.payload[4] = 0x04;
        RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    router.processPacket(pkt, rxInfo);
        TEST_ASSERT_FALSE(events.forwardPacketCalled);
}

void test_packet_router_forwards_direct_when_next_hop() {
    PacketHistory history;
    RepeaterConfig config;
    config.enabled = true;
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Request);
    pkt.pathLen = 3;
    pkt.path[0] = 0xAA;
    pkt.path[1] = 0xBB;
    pkt.path[2] = 0xCC;
    pkt.payloadLen = 2;
    pkt.payload[0] = 0xDD;
    pkt.payload[1] = 0xEE;
        RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    router.processPacket(pkt, rxInfo);
        TEST_ASSERT_TRUE(events.forwardPacketCalled);

    TEST_ASSERT_EQUAL_UINT8(2, events.lastForwardPacket.pathLen);
    TEST_ASSERT_EQUAL_UINT8(0xBB, events.lastForwardPacket.path[0]);
}
void test_packet_router_drops_direct_when_not_next_hop() {
    PacketHistory history;
    RepeaterConfig config;
    config.enabled = true;
    Repeater repeater{history, config};
    repeater.setSelfHash(0xAA);
    TestPacketRouterEvents events;
    PacketRouter router{repeater};
    router.setEventHandler(&events);
    router.setSelfHash(0xAA);
        Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Request);
    pkt.pathLen = 2;
    pkt.path[0] = 0xBB;
    pkt.path[1] = 0xCC;
    pkt.payloadLen = 2;
    pkt.payload[0] = 0xDD;
    pkt.payload[1] = 0xEE;
        RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    router.processPacket(pkt, rxInfo);
        TEST_ASSERT_FALSE(events.forwardPacketCalled);
}

void test_packet_router_does_not_forward_control_packets() {
    PacketHistory history;
    RepeaterConfig config;
    config.enabled = true;
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
    router.processPacket(pkt, rxInfo);
        TEST_ASSERT_FALSE(events.forwardPacketCalled);
}

void test_packet_router_calculates_correct_priority() {
    PacketHistory history;
    RepeaterConfig config;
    config.enabled = true;
    config.maxFloodPath = 64;
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
    pkt.payload[1] = 0x01;
    pkt.payload[2] = 0x02;
    pkt.payload[3] = 0x03;
    pkt.payload[4] = 0x04;
        RxPacket rxInfo{pkt.payload, static_cast<uint8_t>(pkt.payloadLen), -50, 10};
    router.processPacket(pkt, rxInfo);
        TEST_ASSERT_TRUE(events.forwardPacketCalled);
    TEST_ASSERT_EQUAL_UINT8(Config::PRIORITY_FLOOD_DEFAULT, events.lastForwardPriority);
}
