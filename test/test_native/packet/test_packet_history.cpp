#include <unity.h>
#include "packet/packet_history.h"
#include <cstring>
using namespace MiniCore;

static Packet makePacket(PayloadType type, const uint8_t* payload, uint16_t len) {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, type);
    pkt.pathLen = 0;
    pkt.payloadLen = len;
    std::memcpy(pkt.payload, payload, len);
    return pkt;
}

static Packet makeAckPacket(uint32_t ackCrc) {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    pkt.pathLen = 0;
    pkt.payloadLen = 4;
    std::memcpy(pkt.payload, &ackCrc, 4);
    return pkt;
}

void test_packet_history_initial_state_is_empty() {
    PacketHistory history;
    TEST_ASSERT_EQUAL_UINT16(0, history.getHashCount());
    TEST_ASSERT_EQUAL_UINT8(0, history.getAckCount());
}
void test_packet_history_has_seen_returns_false_for_new_packet() {
    PacketHistory history;
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        TEST_ASSERT_FALSE(history.hasSeen(pkt));
}
void test_packet_history_has_seen_adds_packet_on_first_check() {
    PacketHistory history;
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        history.hasSeen(pkt);
        TEST_ASSERT_EQUAL_UINT16(1, history.getHashCount());
}
void test_packet_history_has_seen_returns_true_for_duplicate() {
    PacketHistory history;
    uint8_t payload[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        history.hasSeen(pkt);
        TEST_ASSERT_TRUE(history.hasSeen(pkt));
}
void test_packet_history_tracks_multiple_unique_packets() {
    PacketHistory history;
        uint8_t payload1[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t payload2[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    uint8_t payload3[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
        Packet pkt1 = makePacket(PayloadType::TextMessage, payload1, 5);
    Packet pkt2 = makePacket(PayloadType::TextMessage, payload2, 5);
    Packet pkt3 = makePacket(PayloadType::TextMessage, payload3, 5);
        history.hasSeen(pkt1);
    history.hasSeen(pkt2);
    history.hasSeen(pkt3);
        TEST_ASSERT_EQUAL_UINT16(3, history.getHashCount());
    TEST_ASSERT_TRUE(history.hasSeen(pkt1));
    TEST_ASSERT_TRUE(history.hasSeen(pkt2));
    TEST_ASSERT_TRUE(history.hasSeen(pkt3));
}
void test_packet_history_duplicate_does_not_increase_count() {
    PacketHistory history;
    uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        history.hasSeen(pkt);
    history.hasSeen(pkt);
    history.hasSeen(pkt);
        TEST_ASSERT_EQUAL_UINT16(1, history.getHashCount());
}
void test_packet_history_clear_resets_state() {
    PacketHistory history;
    uint8_t payload[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        history.hasSeen(pkt);
    history.clearAll();
        TEST_ASSERT_EQUAL_UINT16(0, history.getHashCount());
    TEST_ASSERT_FALSE(history.hasSeen(pkt));
}

void test_packet_history_ack_uses_payload_directly() {
    PacketHistory history;

    Packet ack1 = makeAckPacket(0x12345678);
    Packet ack2 = makeAckPacket(0x12345678);
        TEST_ASSERT_FALSE(history.hasSeen(ack1));
    TEST_ASSERT_TRUE(history.hasSeen(ack2));
}
void test_packet_history_ack_stored_separately() {
    PacketHistory history;
        Packet ack = makeAckPacket(0xDEADBEEF);
    history.hasSeen(ack);
        TEST_ASSERT_EQUAL_UINT8(1, history.getAckCount());
    TEST_ASSERT_EQUAL_UINT16(0, history.getHashCount());
}
void test_packet_history_ack_different_crcs_not_duplicates() {
    PacketHistory history;
        Packet ack1 = makeAckPacket(0x11111111);
    Packet ack2 = makeAckPacket(0x22222222);
    Packet ack3 = makeAckPacket(0x33333333);
        TEST_ASSERT_FALSE(history.hasSeen(ack1));
    TEST_ASSERT_FALSE(history.hasSeen(ack2));
    TEST_ASSERT_FALSE(history.hasSeen(ack3));
    TEST_ASSERT_EQUAL_UINT8(3, history.getAckCount());
}
void test_packet_history_ack_cleared_separately() {
    PacketHistory history;
        Packet ack = makeAckPacket(0xCAFEBABE);
    history.hasSeen(ack);
    TEST_ASSERT_TRUE(history.hasSeen(ack));
        history.clear(ack);
    TEST_ASSERT_FALSE(history.hasSeen(ack));
}
void test_packet_history_ack_max_capacity() {
    PacketHistory history;

    for (uint32_t i = 0; i < Config::MAX_PACKET_ACKS; ++i) {
        Packet ack = makeAckPacket(i + 1);
        history.hasSeen(ack);
    }
        TEST_ASSERT_EQUAL_UINT8(Config::MAX_PACKET_ACKS, history.getAckCount());
}
void test_packet_history_ack_eviction_is_cyclic() {
    PacketHistory history;

    for (uint32_t i = 0; i < Config::MAX_PACKET_ACKS; ++i) {
        Packet ack = makeAckPacket(0x10000 + i);
        history.hasSeen(ack);
    }

    Packet newAck = makeAckPacket(0xFFFFFFFF);
    history.hasSeen(newAck);

    Packet firstAck = makeAckPacket(0x10000);
    TEST_ASSERT_FALSE(history.hasSeen(firstAck));
}

void test_packet_history_linear_search_finds_all_entries() {
    PacketHistory history;

    for (uint16_t i = 0; i < 50; ++i) {
        uint8_t payload[5] = {
            static_cast<uint8_t>(i & 0xFF),
            static_cast<uint8_t>((i >> 8) & 0xFF),
            0xAA, 0xBB, 0xCC
        };
        Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        history.hasSeen(pkt);
    }

    for (uint16_t i = 0; i < 50; ++i) {
        uint8_t payload[5] = {
            static_cast<uint8_t>(i & 0xFF),
            static_cast<uint8_t>((i >> 8) & 0xFF),
            0xAA, 0xBB, 0xCC
        };
        Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        TEST_ASSERT_TRUE(history.hasSeen(pkt));
    }
}
void test_packet_history_handles_max_capacity() {
    PacketHistory history;

    for (uint16_t i = 0; i < Config::MAX_PACKET_HASHES; ++i) {
        uint8_t payload[5] = {
            static_cast<uint8_t>(i & 0xFF),
            static_cast<uint8_t>((i >> 8) & 0xFF),
            0x11, 0x22, 0x33
        };
        Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        history.hasSeen(pkt);
    }
        TEST_ASSERT_EQUAL_UINT16(Config::MAX_PACKET_HASHES, history.getHashCount());
}
void test_packet_history_cyclic_eviction() {
    PacketHistory history;

    for (uint16_t i = 0; i < Config::MAX_PACKET_HASHES; ++i) {
        uint8_t payload[5] = {
            static_cast<uint8_t>(i & 0xFF),
            static_cast<uint8_t>((i >> 8) & 0xFF),
            0xAA, 0xBB, 0xCC
        };
        Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        history.hasSeen(pkt);
    }

    uint8_t newPayload[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    Packet newPkt = makePacket(PayloadType::TextMessage, newPayload, 5);
    history.hasSeen(newPkt);

    uint8_t firstPayload[5] = {0x00, 0x00, 0xAA, 0xBB, 0xCC};
    Packet firstPkt = makePacket(PayloadType::TextMessage, firstPayload, 5);
    TEST_ASSERT_FALSE(history.hasSeen(firstPkt));
}

void test_packet_history_calculate_hash_from_packet() {
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 0;
    pkt.payloadLen = 10;
    std::memset(pkt.payload, 0xAB, 10);
        uint8_t hash1[Config::MAX_HASH_SIZE];
    uint8_t hash2[Config::MAX_HASH_SIZE];
        PacketHistory::calculateHash(pkt, hash1);
    PacketHistory::calculateHash(pkt, hash2);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(hash1, hash2, Config::MAX_HASH_SIZE);
}
void test_packet_history_different_packets_have_different_hashes() {
    Packet pkt1;
    pkt1.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt1.pathLen = 0;
    pkt1.payloadLen = 5;
    std::memset(pkt1.payload, 0x11, 5);
        Packet pkt2;
    pkt2.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt2.pathLen = 0;
    pkt2.payloadLen = 5;
    std::memset(pkt2.payload, 0x22, 5);
        uint8_t hash1[Config::MAX_HASH_SIZE];
    uint8_t hash2[Config::MAX_HASH_SIZE];
        PacketHistory::calculateHash(pkt1, hash1);
    PacketHistory::calculateHash(pkt2, hash2);
        TEST_ASSERT_FALSE(std::memcmp(hash1, hash2, Config::MAX_HASH_SIZE) == 0);
}
void test_packet_history_hash_ignores_path_for_flood() {
    Packet pkt1;
    pkt1.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt1.pathLen = 0;
    pkt1.payloadLen = 5;
    std::memset(pkt1.payload, 0xAA, 5);
        Packet pkt2;
    pkt2.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt2.pathLen = 2;
    pkt2.path[0] = 0x12;
    pkt2.path[1] = 0x34;
    pkt2.payloadLen = 5;
    std::memset(pkt2.payload, 0xAA, 5);
        uint8_t hash1[Config::MAX_HASH_SIZE];
    uint8_t hash2[Config::MAX_HASH_SIZE];
        PacketHistory::calculateHash(pkt1, hash1);
    PacketHistory::calculateHash(pkt2, hash2);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(hash1, hash2, Config::MAX_HASH_SIZE);
}
void test_packet_hash_only_uses_payload_type_not_full_header() {
    Packet pkt1;
    pkt1.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt1.pathLen = 0;
    pkt1.payloadLen = 5;
    std::memset(pkt1.payload, 0xAA, 5);
        Packet pkt2;
    pkt2.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    pkt2.pathLen = 0;
    pkt2.payloadLen = 5;
    std::memset(pkt2.payload, 0xAA, 5);
        uint8_t hash1[Config::MAX_HASH_SIZE];
    uint8_t hash2[Config::MAX_HASH_SIZE];
        PacketHistory::calculateHash(pkt1, hash1);
    PacketHistory::calculateHash(pkt2, hash2);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(hash1, hash2, Config::MAX_HASH_SIZE);
}
void test_packet_hash_different_payload_types_differ() {
    Packet pkt1;
    pkt1.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt1.pathLen = 0;
    pkt1.payloadLen = 5;
    std::memset(pkt1.payload, 0xAA, 5);
        Packet pkt2;
    pkt2.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt2.pathLen = 0;
    pkt2.payloadLen = 5;
    std::memset(pkt2.payload, 0xAA, 5);
        uint8_t hash1[Config::MAX_HASH_SIZE];
    uint8_t hash2[Config::MAX_HASH_SIZE];
        PacketHistory::calculateHash(pkt1, hash1);
    PacketHistory::calculateHash(pkt2, hash2);
        TEST_ASSERT_FALSE(std::memcmp(hash1, hash2, Config::MAX_HASH_SIZE) == 0);
}
void test_packet_hash_trace_includes_path_len() {
    Packet pkt1;
    pkt1.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    pkt1.pathLen = 0;
    pkt1.payloadLen = 12;
    std::memset(pkt1.payload, 0x55, 12);
        Packet pkt2;
    pkt2.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    pkt2.pathLen = 2;
    pkt2.payloadLen = 12;
    std::memset(pkt2.payload, 0x55, 12);
        uint8_t hash1[Config::MAX_HASH_SIZE];
    uint8_t hash2[Config::MAX_HASH_SIZE];
        PacketHistory::calculateHash(pkt1, hash1);
    PacketHistory::calculateHash(pkt2, hash2);
        TEST_ASSERT_FALSE(std::memcmp(hash1, hash2, Config::MAX_HASH_SIZE) == 0);
}
void test_packet_hash_non_trace_ignores_path_len() {
    Packet pkt1;
    pkt1.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    pkt1.pathLen = 0;
    pkt1.payloadLen = 5;
    std::memset(pkt1.payload, 0x55, 5);
        Packet pkt2;
    pkt2.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
    pkt2.pathLen = 3;
    pkt2.payloadLen = 5;
    std::memset(pkt2.payload, 0x55, 5);
        uint8_t hash1[Config::MAX_HASH_SIZE];
    uint8_t hash2[Config::MAX_HASH_SIZE];
        PacketHistory::calculateHash(pkt1, hash1);
    PacketHistory::calculateHash(pkt2, hash2);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(hash1, hash2, Config::MAX_HASH_SIZE);
}

void test_packet_history_contains_returns_false_for_new_packet() {
    PacketHistory history;
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        TEST_ASSERT_FALSE(history.contains(pkt));
}
void test_packet_history_contains_returns_true_after_has_seen() {
    PacketHistory history;
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        history.hasSeen(pkt);
        TEST_ASSERT_TRUE(history.contains(pkt));
}
void test_packet_history_contains_does_not_add_to_history() {
    PacketHistory history;
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);

    history.contains(pkt);
    history.contains(pkt);
    history.contains(pkt);

    TEST_ASSERT_EQUAL_UINT16(0, history.getHashCount());
}
void test_packet_history_contains_works_for_ack() {
    PacketHistory history;
    Packet ack = makeAckPacket(0xDEADBEEF);
        TEST_ASSERT_FALSE(history.contains(ack));
        history.hasSeen(ack);
        TEST_ASSERT_TRUE(history.contains(ack));
}

void test_packet_history_clear_removes_specific_packet() {
    PacketHistory history;
        uint8_t payload1[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    uint8_t payload2[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    Packet pkt1 = makePacket(PayloadType::TextMessage, payload1, 5);
    Packet pkt2 = makePacket(PayloadType::TextMessage, payload2, 5);
        history.hasSeen(pkt1);
    history.hasSeen(pkt2);

    history.clear(pkt1);

    TEST_ASSERT_FALSE(history.contains(pkt1));
    TEST_ASSERT_TRUE(history.contains(pkt2));
}

void test_packet_history_tracks_duplicate_stats() {
    PacketHistory history;
        uint8_t payload[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
        history.hasSeen(pkt);
    history.hasSeen(pkt);
    history.hasSeen(pkt);
        TEST_ASSERT_EQUAL_UINT32(2, history.getFloodDups());
    TEST_ASSERT_EQUAL_UINT32(0, history.getDirectDups());
}
void test_packet_history_tracks_direct_duplicate_stats() {
    PacketHistory history;
        uint8_t payload[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::TextMessage);
        history.hasSeen(pkt);
    history.hasSeen(pkt);
        TEST_ASSERT_EQUAL_UINT32(0, history.getFloodDups());
    TEST_ASSERT_EQUAL_UINT32(1, history.getDirectDups());
}
void test_packet_history_reset_stats() {
    PacketHistory history;
        uint8_t payload[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    Packet pkt = makePacket(PayloadType::TextMessage, payload, 5);
        history.hasSeen(pkt);
    history.hasSeen(pkt);
    history.resetStats();
        TEST_ASSERT_EQUAL_UINT32(0, history.getFloodDups());
    TEST_ASSERT_EQUAL_UINT32(0, history.getDirectDups());
}
