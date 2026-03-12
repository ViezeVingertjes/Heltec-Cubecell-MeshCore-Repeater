#pragma once
#include <cstdint>
#include <cstddef>
#include "core/config.h"
#include "packet/packet.h"
#include "core/result.h"
namespace MiniCore {
using Config::TX_QUEUE_SIZE;
struct TxEntry {
    uint8_t data[MAX_MTU_SIZE];
    uint8_t length;
    uint32_t scheduledTime;
    uint8_t priority;
    bool active;
};
class TxQueue {
public:
    TxQueue();
    Status enqueue(const uint8_t* data, uint16_t length, uint32_t scheduledTime, uint8_t priority);
    Status enqueuePacket(const Packet& packet, uint32_t scheduledTime, uint8_t priority);
    [[nodiscard]] TxEntry* getNextReady(uint32_t currentTime);
    void release(TxEntry* entry);
    void clear();
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] uint8_t count() const;
private:
    TxEntry entries_[TX_QUEUE_SIZE];
        [[nodiscard]] bool isTimeReady(uint32_t scheduledTime, uint32_t currentTime) const;
};
}
