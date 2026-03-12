#include <unity.h>
#include "queue/tx_queue.h"
#include "packet/packet.h"
#include "core/result.h"
#include <cstring>
using namespace MiniCore;
void test_tx_queue_initial_state_is_empty() {
    TxQueue queue;
    TEST_ASSERT_TRUE(queue.isEmpty());
    TEST_ASSERT_EQUAL_UINT8(0, queue.count());
}
void test_tx_queue_enqueue_single_packet() {
    TxQueue queue;
    uint8_t data[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    auto status = queue.enqueue(data, 10, 1000, 0);
    TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_FALSE(queue.isEmpty());
    TEST_ASSERT_EQUAL_UINT8(1, queue.count());
}
void test_tx_queue_enqueue_rejects_null_data_when_length_nonzero() {
    TxQueue queue;
    auto status = queue.enqueue(nullptr, 5, 1000, 0);
    TEST_ASSERT_FALSE(status.isOk());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::InvalidParameter), static_cast<int>(status.error()));
}
void test_tx_queue_enqueue_multiple_packets() {
    TxQueue queue;
    uint8_t data1[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
    uint8_t data2[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
        TEST_ASSERT_TRUE(queue.enqueue(data1, 5, 1000, 0).isOk());
    TEST_ASSERT_TRUE(queue.enqueue(data2, 5, 2000, 0).isOk());
        TEST_ASSERT_EQUAL_UINT8(2, queue.count());
}
void test_tx_queue_get_next_returns_null_when_empty() {
    TxQueue queue;
        auto* entry = queue.getNextReady(1000);
        TEST_ASSERT_NULL(entry);
}
void test_tx_queue_get_next_returns_null_when_not_ready() {
    TxQueue queue;
    uint8_t data[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
        TEST_ASSERT_TRUE(queue.enqueue(data, 5, 2000, 0).isOk());
        auto* entry = queue.getNextReady(1000);
        TEST_ASSERT_NULL(entry);
}
void test_tx_queue_get_next_returns_ready_packet() {
    TxQueue queue;
    uint8_t data[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
        TEST_ASSERT_TRUE(queue.enqueue(data, 5, 1000, 0).isOk());
        auto* entry = queue.getNextReady(1500);
        TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT8(5, entry->length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, entry->data, 5);
}
void test_tx_queue_release_removes_entry() {
    TxQueue queue;
    uint8_t data[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
        TEST_ASSERT_TRUE(queue.enqueue(data, 5, 1000, 0).isOk());
    auto* entry = queue.getNextReady(1500);
        queue.release(entry);
        TEST_ASSERT_TRUE(queue.isEmpty());
}
void test_tx_queue_priority_ordering_lower_first() {
    TxQueue queue;
    uint8_t data1[3] = {0x01, 0x02, 0x03};
    uint8_t data2[3] = {0x0A, 0x0B, 0x0C};
    uint8_t data3[3] = {0xF1, 0xF2, 0xF3};
        TEST_ASSERT_TRUE(queue.enqueue(data1, 3, 1000, 5).isOk());
    TEST_ASSERT_TRUE(queue.enqueue(data2, 3, 1000, 1).isOk());
    TEST_ASSERT_TRUE(queue.enqueue(data3, 3, 1000, 3).isOk());
        auto* entry = queue.getNextReady(1500);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT8(0x0A, entry->data[0]);
}
void test_tx_queue_time_ordering_when_same_priority() {
    TxQueue queue;
    uint8_t data1[3] = {0x01, 0x02, 0x03};
    uint8_t data2[3] = {0x0A, 0x0B, 0x0C};
        TEST_ASSERT_TRUE(queue.enqueue(data1, 3, 2000, 0).isOk());
    TEST_ASSERT_TRUE(queue.enqueue(data2, 3, 1000, 0).isOk());
        auto* entry = queue.getNextReady(2500);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT8(0x0A, entry->data[0]);
}
void test_tx_queue_respects_scheduled_time() {
    TxQueue queue;
    uint8_t data1[3] = {0x01, 0x02, 0x03};
    uint8_t data2[3] = {0x0A, 0x0B, 0x0C};
        TEST_ASSERT_TRUE(queue.enqueue(data1, 3, 3000, 0).isOk());
    TEST_ASSERT_TRUE(queue.enqueue(data2, 3, 1000, 0).isOk());
        auto* entry = queue.getNextReady(1500);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT8(0x0A, entry->data[0]);
        queue.release(entry);
        entry = queue.getNextReady(1500);
    TEST_ASSERT_NULL(entry);
        entry = queue.getNextReady(3500);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT8(0x01, entry->data[0]);
}
void test_tx_queue_full_returns_false_on_enqueue() {
    TxQueue queue;
    uint8_t data[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
        for (uint8_t i = 0; i < TX_QUEUE_SIZE; ++i) {
        auto s = queue.enqueue(data, 5, 1000 + i, 0);
        TEST_ASSERT_TRUE_MESSAGE(s.isOk(), "enqueue when not full should succeed");
    }
        auto status = queue.enqueue(data, 5, 9999, 0);
    TEST_ASSERT_FALSE(status.isOk());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::QueueFull), static_cast<int>(status.error()));
}
void test_tx_queue_clear_empties_queue() {
    TxQueue queue;
    uint8_t data[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
        TEST_ASSERT_TRUE(queue.enqueue(data, 5, 1000, 0).isOk());
    TEST_ASSERT_TRUE(queue.enqueue(data, 5, 2000, 0).isOk());
        queue.clear();
        TEST_ASSERT_TRUE(queue.isEmpty());
    TEST_ASSERT_EQUAL_UINT8(0, queue.count());
}
void test_tx_queue_handles_timestamp_wraparound() {
    TxQueue queue;
    uint8_t data[3] = {0x01, 0x02, 0x03};
        TEST_ASSERT_TRUE(queue.enqueue(data, 3, 0xFFFFFFF0, 0).isOk());
        auto* entry = queue.getNextReady(0x00000010);
    TEST_ASSERT_NOT_NULL(entry);
}
void test_tx_queue_packet_too_large_rejected() {
    TxQueue queue;
    uint8_t data[MAX_MTU_SIZE + 1];
    std::memset(data, 0xAB, sizeof(data));
        auto status = queue.enqueue(data, MAX_MTU_SIZE + 1, 1000, 0);
        TEST_ASSERT_FALSE(status.isOk());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::BufferTooSmall), static_cast<int>(status.error()));
    TEST_ASSERT_TRUE(queue.isEmpty());
}

void test_tx_queue_enqueue_packet_encodes_and_stores() {
    TxQueue queue;
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 0;
    pkt.payloadLen = 3;
    pkt.payload[0] = 0xAA;
    pkt.payload[1] = 0xBB;
    pkt.payload[2] = 0xCC;
        auto status = queue.enqueuePacket(pkt, 1000, 5);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(1, queue.count());
        auto* entry = queue.getNextReady(1500);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT8(5, entry->priority);
}
void test_tx_queue_enqueue_packet_stores_correct_encoded_data() {
    TxQueue queue;
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    pkt.pathLen = 2;
    pkt.path[0] = 0x11;
    pkt.path[1] = 0x22;
    pkt.payloadLen = 2;
    pkt.payload[0] = 0xDE;
    pkt.payload[1] = 0xAD;
        TEST_ASSERT_TRUE(queue.enqueuePacket(pkt, 2000, 0).isOk());
        auto* entry = queue.getNextReady(2500);
    TEST_ASSERT_NOT_NULL(entry);

    Packet decoded;
    auto status = decodePacket(entry->data, entry->length, decoded);
    TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL(pkt.header, decoded.header);
    TEST_ASSERT_EQUAL_UINT8(2, decoded.pathLen);
    TEST_ASSERT_EQUAL_UINT8(0x11, decoded.path[0]);
    TEST_ASSERT_EQUAL_UINT8(0x22, decoded.path[1]);
    TEST_ASSERT_EQUAL_UINT16(2, decoded.payloadLen);
    TEST_ASSERT_EQUAL_UINT8(0xDE, decoded.payload[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAD, decoded.payload[1]);
}
void test_tx_queue_enqueue_packet_respects_scheduled_time() {
    TxQueue queue;
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Ack);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x42;
        TEST_ASSERT_TRUE(queue.enqueuePacket(pkt, 5000, 0).isOk());
        auto* entry = queue.getNextReady(4000);
    TEST_ASSERT_NULL(entry);
        entry = queue.getNextReady(5500);
    TEST_ASSERT_NOT_NULL(entry);
}
void test_tx_queue_enqueue_packet_returns_false_when_full() {
    TxQueue queue;
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    pkt.pathLen = 0;
    pkt.payloadLen = 1;
    pkt.payload[0] = 0x01;

    for (uint8_t i = 0; i < TX_QUEUE_SIZE; ++i) {
        TEST_ASSERT_TRUE(queue.enqueuePacket(pkt, 1000 + i, 0).isOk());
    }

    auto status = queue.enqueuePacket(pkt, 9999, 0);
    TEST_ASSERT_FALSE(status.isOk());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::QueueFull), static_cast<int>(status.error()));
}
void test_tx_queue_enqueue_packet_handles_empty_payload() {
    TxQueue queue;
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Ack);
    pkt.pathLen = 1;
    pkt.path[0] = 0xAB;
    pkt.payloadLen = 0;
        auto status = queue.enqueuePacket(pkt, 1000, 0);
        TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_EQUAL_UINT8(1, queue.count());
}
void test_tx_queue_enqueue_packet_with_transport_codes() {
    TxQueue queue;
    Packet pkt;
    pkt.header = Packet::makeHeader(RouteType::TransportFlood, PayloadType::Request);
    pkt.transportCodes[0] = 0x1234;
    pkt.transportCodes[1] = 0x5678;
    pkt.pathLen = 0;
    pkt.payloadLen = 2;
    pkt.payload[0] = 0xAA;
    pkt.payload[1] = 0xBB;
        TEST_ASSERT_TRUE(queue.enqueuePacket(pkt, 1000, 0).isOk());
        auto* entry = queue.getNextReady(1500);
    TEST_ASSERT_NOT_NULL(entry);
        Packet decoded;
    auto status = decodePacket(entry->data, entry->length, decoded);
    TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_TRUE(decoded.hasTransportCodes());
    TEST_ASSERT_EQUAL_UINT16(0x1234, decoded.transportCodes[0]);
    TEST_ASSERT_EQUAL_UINT16(0x5678, decoded.transportCodes[1]);
}
void test_tx_queue_trace_packet_enqueues_normally() {
    TxQueue queue;
    Packet tracePkt;
    tracePkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Trace);
    tracePkt.pathLen = 1;
    tracePkt.path[0] = 0xAB;
    tracePkt.payloadLen = 10;
        TEST_ASSERT_TRUE(queue.enqueuePacket(tracePkt, 0, 0).isOk());
        auto* entry = queue.getNextReady(100);
    TEST_ASSERT_NOT_NULL(entry);

}
void test_tx_queue_all_packets_use_carrier_sense() {
    TxQueue queue;
    Packet textPkt;
    textPkt.header = Packet::makeHeader(RouteType::Flood, PayloadType::TextMessage);
    textPkt.pathLen = 0;
    textPkt.payloadLen = 5;
        TEST_ASSERT_TRUE(queue.enqueuePacket(textPkt, 0, 0).isOk());
        auto* entry = queue.getNextReady(100);
    TEST_ASSERT_NOT_NULL(entry);

}
