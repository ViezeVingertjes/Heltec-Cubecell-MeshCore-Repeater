#include "queue/tx_queue.h"
#include <cstring>
namespace MiniCore {
TxQueue::TxQueue()
    : entries_{}
{
    clear();
}
Status TxQueue::enqueue(const uint8_t* data, uint16_t length, uint32_t scheduledTime, uint8_t priority) {
    if (length > MAX_MTU_SIZE) {
        return ErrorCode::BufferTooSmall;
    }
    if (length > 0 && data == nullptr) {
        return ErrorCode::InvalidParameter;
    }
    for (auto& entry : entries_) {
        if (!entry.active) {
            std::memcpy(entry.data, data, length);
            entry.length = length;
            entry.scheduledTime = scheduledTime;
            entry.priority = priority;
            entry.active = true;
            return Status();
        }
    }
        return ErrorCode::QueueFull;
}
Status TxQueue::enqueuePacket(const Packet& packet, uint32_t scheduledTime, uint8_t priority) {
    uint8_t buffer[MAX_MTU_SIZE];
    uint8_t length = 0;
        auto status = encodePacket(packet, buffer, length);
    if (!status.isOk()) {
        return status.error();
    }
        return enqueue(buffer, length, scheduledTime, priority);
}
TxEntry* TxQueue::getNextReady(uint32_t currentTime) {
    TxEntry* best = nullptr;
        for (auto& entry : entries_) {
        if (!entry.active) {
            continue;
        }
                if (!isTimeReady(entry.scheduledTime, currentTime)) {
            continue;
        }
                if (best == nullptr) {
            best = &entry;
        } else if (entry.priority < best->priority) {
            best = &entry;
        } else if (entry.priority == best->priority && 
                   isTimeReady(entry.scheduledTime, best->scheduledTime)) {
            best = &entry;
        }
    }
        return best;
}
void TxQueue::release(TxEntry* entry) {
    if (entry != nullptr) {
        entry->active = false;
    }
}
void TxQueue::clear() {
    for (auto& entry : entries_) {
        entry.active = false;
        entry.length = 0;
        entry.scheduledTime = 0;
        entry.priority = 0;
    }
}
bool TxQueue::isEmpty() const {
    for (const auto& entry : entries_) {
        if (entry.active) {
            return false;
        }
    }
    return true;
}
uint8_t TxQueue::count() const {
    uint8_t cnt = 0;
    for (const auto& entry : entries_) {
        if (entry.active) {
            ++cnt;
        }
    }
    return cnt;
}
bool TxQueue::isTimeReady(uint32_t scheduledTime, uint32_t currentTime) const {
    int32_t diff = static_cast<int32_t>(currentTime - scheduledTime);
    return diff >= 0;
}
}
