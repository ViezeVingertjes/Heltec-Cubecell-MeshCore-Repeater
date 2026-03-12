#include <unity.h>
#include "packet/packet_history.h"
#include <cstring>
using namespace MiniCore;

static Packet makePacket(PayloadType type, RouteType route, const uint8_t* payload, uint16_t len) {
    Packet pkt;
    pkt.header = Packet::makeHeader(route, type);
    pkt.pathLen = 0;
    pkt.payloadLen = len;
    if (payload && len > 0) {
        std::memcpy(pkt.payload, payload, len);
    }
    return pkt;
}
static Packet makeAckPacket(uint32_t ackCrc, RouteType route = RouteType::Flood) {
    Packet pkt;
    pkt.header = Packet::makeHeader(route, PayloadType::Ack);
    pkt.pathLen = 0;
    pkt.payloadLen = 4;
    std::memcpy(pkt.payload, &ackCrc, 4);
    return pkt;
}

void test_edge_same_packet_detected_on_every_check() {
    PacketHistory history;
    uint8_t payload[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    Packet pkt = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 8);

    TEST_ASSERT_FALSE(history.hasSeen(pkt));

    for (int i = 0; i < 1000; i++) {
        TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt), "Duplicate not detected!");
    }
}
void test_edge_same_packet_different_route_type_is_duplicate() {

    PacketHistory history;
    uint8_t payload[] = {0xAA, 0xBB, 0xCC, 0xDD};
        Packet pktFlood = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 4);
    Packet pktDirect = makePacket(PayloadType::TextMessage, RouteType::Direct, payload, 4);

    TEST_ASSERT_FALSE(history.hasSeen(pktFlood));

    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pktDirect), 
        "Same packet via different route must be duplicate!");
}
void test_edge_same_packet_with_different_path_is_duplicate() {

    PacketHistory history;
    uint8_t payload[] = {0x12, 0x34, 0x56, 0x78};
        Packet pkt1 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 4);
    pkt1.pathLen = 0;
        Packet pkt2 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 4);
    pkt2.pathLen = 3;
    pkt2.path[0] = 0xAA;
    pkt2.path[1] = 0xBB;
    pkt2.path[2] = 0xCC;
        TEST_ASSERT_FALSE(history.hasSeen(pkt1));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt2), 
        "Same payload with different path must be duplicate!");
}
void test_edge_same_packet_with_different_snr_is_duplicate() {
    PacketHistory history;
    uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
        Packet pkt1 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 4);
    pkt1.snr = 10;
        Packet pkt2 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 4);
    pkt2.snr = -20;
        TEST_ASSERT_FALSE(history.hasSeen(pkt1));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt2), 
        "Same payload with different SNR must be duplicate!");
}
void test_edge_same_packet_with_transport_codes_is_duplicate() {
    PacketHistory history;
    uint8_t payload[] = {0xCA, 0xFE, 0xBA, 0xBE};
        Packet pkt1 = makePacket(PayloadType::TextMessage, RouteType::TransportFlood, payload, 4);
    pkt1.transportCodes[0] = 0x1111;
    pkt1.transportCodes[1] = 0x2222;
        Packet pkt2 = makePacket(PayloadType::TextMessage, RouteType::TransportFlood, payload, 4);
    pkt2.transportCodes[0] = 0x3333;
    pkt2.transportCodes[1] = 0x4444;
        TEST_ASSERT_FALSE(history.hasSeen(pkt1));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt2), 
        "Same payload with different transport codes must be duplicate!");
}

void test_edge_single_byte_difference_not_duplicate() {
    PacketHistory history;
    uint8_t payload1[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t payload2[] = {0x00, 0x00, 0x00, 0x00, 0x01};
        Packet pkt1 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload1, 5);
    Packet pkt2 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload2, 5);
        TEST_ASSERT_FALSE(history.hasSeen(pkt1));
    TEST_ASSERT_FALSE_MESSAGE(history.hasSeen(pkt2), 
        "Single byte difference must not be duplicate!");
}
void test_edge_different_payload_length_not_duplicate() {
    PacketHistory history;
    uint8_t payload[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
        Packet pkt1 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 4);
    Packet pkt2 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 6);
        TEST_ASSERT_FALSE(history.hasSeen(pkt1));
    TEST_ASSERT_FALSE_MESSAGE(history.hasSeen(pkt2), 
        "Different payload length must not be duplicate!");
}
void test_edge_different_payload_type_not_duplicate() {
    PacketHistory history;
    uint8_t payload[] = {0x11, 0x22, 0x33, 0x44};
        Packet pkt1 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 4);
    Packet pkt2 = makePacket(PayloadType::GroupText, RouteType::Flood, payload, 4);
        TEST_ASSERT_FALSE(history.hasSeen(pkt1));
    TEST_ASSERT_FALSE_MESSAGE(history.hasSeen(pkt2), 
        "Different payload type must not be duplicate!");
}

void test_edge_ack_same_crc_always_duplicate() {
    PacketHistory history;
    Packet ack = makeAckPacket(0xDEADBEEF);
        TEST_ASSERT_FALSE(history.hasSeen(ack));

    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(ack), "ACK duplicate not detected!");
    }
}
void test_edge_ack_different_route_is_duplicate() {
    PacketHistory history;
        Packet ackFlood = makeAckPacket(0x12345678, RouteType::Flood);
    Packet ackDirect = makeAckPacket(0x12345678, RouteType::Direct);
        TEST_ASSERT_FALSE(history.hasSeen(ackFlood));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(ackDirect), 
        "Same ACK via different route must be duplicate!");
}
void test_edge_ack_with_extra_payload_bytes_still_uses_first_4() {
    PacketHistory history;

    Packet ack1;
    ack1.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    ack1.pathLen = 0;
    ack1.payloadLen = 4;
    uint32_t crc = 0xCAFEBABE;
    std::memcpy(ack1.payload, &crc, 4);

    Packet ack2;
    ack2.header = Packet::makeHeader(RouteType::Flood, PayloadType::Ack);
    ack2.pathLen = 0;
    ack2.payloadLen = 8;
    std::memcpy(ack2.payload, &crc, 4);
    ack2.payload[4] = 0xFF;
    ack2.payload[5] = 0xEE;
    ack2.payload[6] = 0xDD;
    ack2.payload[7] = 0xCC;
        TEST_ASSERT_FALSE(history.hasSeen(ack1));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(ack2), 
        "ACK with same CRC but extra bytes must be duplicate!");
}
void test_edge_ack_vs_non_ack_same_payload_not_duplicate() {

    PacketHistory history;
        uint32_t data = 0x12345678;
    uint8_t payload[4];
    std::memcpy(payload, &data, 4);
        Packet ack = makeAckPacket(data);
    Packet textMsg = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 4);
        TEST_ASSERT_FALSE(history.hasSeen(ack));
    TEST_ASSERT_FALSE_MESSAGE(history.hasSeen(textMsg), 
        "ACK and non-ACK with same bytes must not be duplicate!");

    TEST_ASSERT_TRUE(history.hasSeen(ack));

    TEST_ASSERT_TRUE(history.hasSeen(textMsg));
}

void test_edge_trace_different_path_len_not_duplicate() {
    PacketHistory history;
    uint8_t payload[12];
    std::memset(payload, 0x55, 12);
        Packet trace1 = makePacket(PayloadType::Trace, RouteType::Direct, payload, 12);
    trace1.pathLen = 0;
        Packet trace2 = makePacket(PayloadType::Trace, RouteType::Direct, payload, 12);
    trace2.pathLen = 1;
        TEST_ASSERT_FALSE(history.hasSeen(trace1));
    TEST_ASSERT_FALSE_MESSAGE(history.hasSeen(trace2), 
        "TRACE with different path_len must not be duplicate!");
}
void test_edge_trace_same_path_len_is_duplicate() {
    PacketHistory history;
    uint8_t payload[12];
    std::memset(payload, 0xAA, 12);
        Packet trace1 = makePacket(PayloadType::Trace, RouteType::Direct, payload, 12);
    trace1.pathLen = 2;
    trace1.path[0] = 0x11;
    trace1.path[1] = 0x22;
        Packet trace2 = makePacket(PayloadType::Trace, RouteType::Direct, payload, 12);
    trace2.pathLen = 2;
    trace2.path[0] = 0x33;
    trace2.path[1] = 0x44;
        TEST_ASSERT_FALSE(history.hasSeen(trace1));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(trace2), 
        "TRACE with same path_len must be duplicate!");
}

void test_edge_packet_at_capacity_boundary_still_found() {
    PacketHistory history;

    for (uint16_t i = 0; i < Config::MAX_PACKET_HASHES - 1; ++i) {
        uint8_t payload[5] = {
            static_cast<uint8_t>(i & 0xFF),
            static_cast<uint8_t>((i >> 8) & 0xFF),
            0xAA, 0xBB, 0xCC
        };
        Packet pkt = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 5);
        history.hasSeen(pkt);
    }

    uint8_t lastPayload[] = {0xFF, 0xFF, 0xAA, 0xBB, 0xCC};
    Packet lastPkt = makePacket(PayloadType::TextMessage, RouteType::Flood, lastPayload, 5);
    TEST_ASSERT_FALSE(history.hasSeen(lastPkt));

    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(lastPkt), 
        "Packet at capacity boundary must be found!");
}
void test_edge_first_packet_evicted_after_overflow() {
    PacketHistory history;

    uint8_t firstPayload[] = {0x00, 0x00, 0xAA, 0xBB, 0xCC};
    Packet firstPkt = makePacket(PayloadType::TextMessage, RouteType::Flood, firstPayload, 5);
    history.hasSeen(firstPkt);

    for (uint16_t i = 1; i < Config::MAX_PACKET_HASHES; ++i) {
        uint8_t payload[5] = {
            static_cast<uint8_t>(i & 0xFF),
            static_cast<uint8_t>((i >> 8) & 0xFF),
            0xAA, 0xBB, 0xCC
        };
        Packet pkt = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 5);
        history.hasSeen(pkt);
    }

    TEST_ASSERT_TRUE(history.contains(firstPkt));

    uint8_t overflowPayload[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    Packet overflowPkt = makePacket(PayloadType::TextMessage, RouteType::Flood, overflowPayload, 5);
    history.hasSeen(overflowPkt);

    TEST_ASSERT_FALSE_MESSAGE(history.contains(firstPkt), 
        "First packet should be evicted after overflow!");
}
void test_edge_recently_evicted_packet_detected_as_new() {
    PacketHistory history;

    uint8_t firstPayload[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    Packet firstPkt = makePacket(PayloadType::TextMessage, RouteType::Flood, firstPayload, 5);
    history.hasSeen(firstPkt);
        for (uint16_t i = 1; i < Config::MAX_PACKET_HASHES; ++i) {

        uint8_t payload[8] = {
            static_cast<uint8_t>((i >> 0) & 0xFF),
            static_cast<uint8_t>((i >> 8) & 0xFF),
            static_cast<uint8_t>(i * 7),
            static_cast<uint8_t>(i * 13),
            static_cast<uint8_t>(i * 17),
            static_cast<uint8_t>(i * 23),
            0x11, 0x22
        };
        Packet pkt = makePacket(PayloadType::Advert, RouteType::Flood, payload, 8);
        history.hasSeen(pkt);
    }

    uint8_t overflowPayload[] = {0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22};
    Packet overflowPkt = makePacket(PayloadType::GroupText, RouteType::Flood, overflowPayload, 8);
    history.hasSeen(overflowPkt);

    TEST_ASSERT_FALSE_MESSAGE(history.contains(firstPkt), 
        "Evicted packet should not be found with contains()!");

    TEST_ASSERT_FALSE_MESSAGE(history.hasSeen(firstPkt), 
        "Evicted packet should be detected as new by hasSeen()!");

    TEST_ASSERT_TRUE_MESSAGE(history.contains(firstPkt),
        "Re-added packet should now be found!");
}
void test_edge_ack_capacity_boundary() {
    PacketHistory history;

    for (uint8_t i = 0; i < Config::MAX_PACKET_ACKS - 1; ++i) {
        Packet ack = makeAckPacket(0x10000 + i);
        history.hasSeen(ack);
    }

    Packet lastAck = makeAckPacket(0xFFFFFFFF);
    TEST_ASSERT_FALSE(history.hasSeen(lastAck));

    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(lastAck), 
        "ACK at capacity boundary must be found!");
}

void test_edge_linear_search_finds_any_position() {
    PacketHistory history;

    Packet packets[50];
    for (int i = 0; i < 50; i++) {
        uint8_t payload[5] = {
            static_cast<uint8_t>(i * 7),
            static_cast<uint8_t>(i),
            0x11, 0x22, 0x33
        };
        packets[i] = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 5);
        history.hasSeen(packets[i]);
    }

    for (int i = 0; i < 50; i++) {
        TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(packets[i]), 
            "Linear search must find packet at any position!");
    }
}
void test_edge_all_same_first_hash_byte_still_unique() {

    PacketHistory history;

    Packet packets[20];
    for (int i = 0; i < 20; i++) {
        uint8_t payload[5] = {
            0xAA,
            static_cast<uint8_t>(i + 1),
            0xCC,
            0xDD,
            0xEE
        };
        packets[i] = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 5);
        history.hasSeen(packets[i]);
    }

    for (int i = 0; i < 20; i++) {
        TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(packets[i]), 
            "Packets with same first hash byte must all be found!");
    }
}

void test_edge_rapid_duplicate_burst() {
    PacketHistory history;
    uint8_t payload[] = {0xBE, 0xEF, 0xCA, 0xFE, 0x00, 0x00, 0x00, 0x00};
    Packet pkt = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 8);

    TEST_ASSERT_FALSE(history.hasSeen(pkt));

    int duplicatesDetected = 0;
    for (int i = 0; i < 100; i++) {
        if (history.hasSeen(pkt)) {
            duplicatesDetected++;
        }
    }
        TEST_ASSERT_EQUAL_MESSAGE(100, duplicatesDetected, 
        "All 100 duplicates must be detected in rapid burst!");
}
void test_edge_interleaved_packets_all_tracked() {
    PacketHistory history;

    uint8_t payload1[] = {0x11, 0x11, 0x11, 0x11};
    uint8_t payload2[] = {0x22, 0x22, 0x22, 0x22};
    uint8_t payload3[] = {0x33, 0x33, 0x33, 0x33};
        Packet pkt1 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload1, 4);
    Packet pkt2 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload2, 4);
    Packet pkt3 = makePacket(PayloadType::TextMessage, RouteType::Flood, payload3, 4);

    TEST_ASSERT_FALSE(history.hasSeen(pkt1));
    TEST_ASSERT_FALSE(history.hasSeen(pkt2));
    TEST_ASSERT_FALSE(history.hasSeen(pkt3));

    for (int round = 0; round < 10; round++) {
        TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt1), "pkt1 duplicate not detected!");
        TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt2), "pkt2 duplicate not detected!");
        TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt3), "pkt3 duplicate not detected!");
        TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt2), "pkt2 duplicate not detected!");
        TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt1), "pkt1 duplicate not detected!");
    }
}

void test_edge_zero_length_payload_tracked() {
    PacketHistory history;
        Packet pkt = makePacket(PayloadType::TextMessage, RouteType::Flood, nullptr, 0);
        TEST_ASSERT_FALSE(history.hasSeen(pkt));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt), 
        "Zero-length payload packet must be tracked!");
}
void test_edge_single_byte_payload_tracked() {
    PacketHistory history;
    uint8_t payload[] = {0x42};
        Packet pkt = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 1);
        TEST_ASSERT_FALSE(history.hasSeen(pkt));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt), 
        "Single-byte payload must be tracked!");
}
void test_edge_all_zeros_payload_tracked() {
    PacketHistory history;
    uint8_t payload[32] = {0};
        Packet pkt = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 32);
        TEST_ASSERT_FALSE(history.hasSeen(pkt));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt), 
        "All-zeros payload must be tracked!");
}
void test_edge_all_ones_payload_tracked() {
    PacketHistory history;
    uint8_t payload[32];
    std::memset(payload, 0xFF, 32);
        Packet pkt = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 32);
        TEST_ASSERT_FALSE(history.hasSeen(pkt));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt), 
        "All-0xFF payload must be tracked!");
}
void test_edge_max_size_payload_tracked() {
    PacketHistory history;
    uint8_t payload[Config::MAX_PACKET_PAYLOAD];
    for (size_t i = 0; i < Config::MAX_PACKET_PAYLOAD; i++) {
        payload[i] = static_cast<uint8_t>(i & 0xFF);
    }
        Packet pkt = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, Config::MAX_PACKET_PAYLOAD);
        TEST_ASSERT_FALSE(history.hasSeen(pkt));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt), 
        "Max-size payload must be tracked!");
}

void test_edge_multipart_ack_detected_as_duplicate() {

    PacketHistory history;
        Packet multiAck;
    multiAck.header = Packet::makeHeader(RouteType::Flood, PayloadType::Multipart);
    multiAck.pathLen = 0;
    multiAck.payloadLen = 5;
    multiAck.payload[0] = (2 << 4) | static_cast<uint8_t>(PayloadType::Ack);
    uint32_t crc = 0xDEADC0DE;
    std::memcpy(&multiAck.payload[1], &crc, 4);
        TEST_ASSERT_FALSE(history.hasSeen(multiAck));
    TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(multiAck), 
        "Multipart ACK must be tracked as duplicate!");
}

void test_edge_all_payload_types_tracked() {
    PacketHistory history;
    uint8_t payload[10];
    std::memset(payload, 0x42, 10);

    PayloadType types[] = {
        PayloadType::Request,
        PayloadType::Response,
        PayloadType::TextMessage,

        PayloadType::Advert,
        PayloadType::GroupText,
        PayloadType::GroupData,
        PayloadType::AnonRequest,
        PayloadType::Path,

        PayloadType::Multipart,
        PayloadType::Control,
        PayloadType::RawCustom
    };

    for (PayloadType type : types) {
        Packet pkt = makePacket(type, RouteType::Flood, payload, 10);
        TEST_ASSERT_FALSE_MESSAGE(history.hasSeen(pkt), 
            "First occurrence should not be duplicate!");
    }

    for (PayloadType type : types) {
        Packet pkt = makePacket(type, RouteType::Flood, payload, 10);
        TEST_ASSERT_TRUE_MESSAGE(history.hasSeen(pkt), 
            "Second occurrence should be duplicate!");
    }
}

void test_edge_contains_never_adds_packet() {
    PacketHistory history;
    uint8_t payload[] = {0xAB, 0xCD, 0xEF};
    Packet pkt = makePacket(PayloadType::TextMessage, RouteType::Flood, payload, 3);

    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_FALSE(history.contains(pkt));
    }

    TEST_ASSERT_EQUAL_UINT16(0, history.getHashCount());

    TEST_ASSERT_FALSE(history.hasSeen(pkt));
}
void test_edge_contains_ack_never_adds() {
    PacketHistory history;
    Packet ack = makeAckPacket(0x11223344);

    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_FALSE(history.contains(ack));
    }

    TEST_ASSERT_EQUAL_UINT8(0, history.getAckCount());
}
