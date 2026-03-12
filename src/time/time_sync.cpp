#include "time/time_sync.h"
#include <cstring>
namespace MiniCore {
TimeSynchronizer::TimeSynchronizer()
    : senders_{}
    , senderCount_(0)
    , sequenceCounter_(0)
{
    reset();
}
void TimeSynchronizer::addSample(uint8_t senderHash, uint32_t receivedTimestamp, uint32_t localTime) {
    const int32_t offset = static_cast<int32_t>(receivedTimestamp) - static_cast<int32_t>(localTime);
    addOrUpdateSample(senderHash, offset);
}
void TimeSynchronizer::addOrUpdateSample(uint8_t senderHash, int32_t offset) {
    int8_t idx = findSender(senderHash);
        if (idx >= 0) {
        senders_[idx].offset = offset;
        senders_[idx].sequence = sequenceCounter_++;
    } else if (senderCount_ < TIME_SYNC_MAX_SENDERS) {
        senders_[senderCount_].senderHash = senderHash;
        senders_[senderCount_].offset = offset;
        senders_[senderCount_].sequence = sequenceCounter_++;
        senders_[senderCount_].active = true;
        ++senderCount_;
    } else {
        uint8_t oldestIdx = findOldestSlot();
        senders_[oldestIdx].senderHash = senderHash;
        senders_[oldestIdx].offset = offset;
        senders_[oldestIdx].sequence = sequenceCounter_++;
    }
}
int32_t TimeSynchronizer::getOffset() const {
    if (senderCount_ == 0) {
        return 0;
    }
    return calculateMedianOffset();
}
bool TimeSynchronizer::hasConsensus() const {
    return senderCount_ >= TIME_SYNC_MIN_CONSENSUS;
}
uint32_t TimeSynchronizer::adjustedTime(uint32_t localTime) const {
    if (!hasConsensus()) {
        return localTime;
    }
    return static_cast<uint32_t>(static_cast<int32_t>(localTime) + getOffset());
}
uint8_t TimeSynchronizer::uniqueSenderCount() const {
    return senderCount_;
}
void TimeSynchronizer::reset() {
    for (auto& sender : senders_) {
        sender.senderHash = 0;
        sender.offset = 0;
        sender.sequence = 0;
        sender.active = false;
    }
    senderCount_ = 0;
    sequenceCounter_ = 0;
}
int32_t TimeSynchronizer::calculateMedianOffset() const {
    if (senderCount_ == 0) {
        return 0;
    }
        uint8_t indices[TIME_SYNC_MAX_SENDERS];
    uint8_t count = 0;
        for (uint8_t i = 0; i < TIME_SYNC_MAX_SENDERS && count < senderCount_; ++i) {
        if (senders_[i].active) {
            indices[count++] = i;
        }
    }
        if (count == 0) {
        return 0;
    }
        for (uint8_t i = 1; i < count; ++i) {
        uint8_t keyIdx = indices[i];
        int32_t keyVal = senders_[keyIdx].offset;
        int8_t j = static_cast<int8_t>(i) - 1;
        while (j >= 0 && senders_[indices[j]].offset > keyVal) {
            indices[j + 1] = indices[j];
            --j;
        }
        indices[j + 1] = keyIdx;
    }
        if (count % 2 == 1) {
        return senders_[indices[count / 2]].offset;
    }
    int32_t a = senders_[indices[count / 2 - 1]].offset;
    int32_t b = senders_[indices[count / 2]].offset;
    return a + (b - a) / 2;
}
int8_t TimeSynchronizer::findSender(uint8_t senderHash) const {
    for (uint8_t i = 0; i < TIME_SYNC_MAX_SENDERS; ++i) {
        if (senders_[i].active && senders_[i].senderHash == senderHash) {
            return static_cast<int8_t>(i);
        }
    }
    return -1;
}
uint8_t TimeSynchronizer::findOldestSlot() const {
    uint8_t oldestIdx = 0;
    uint16_t oldestSeq = senders_[0].sequence;
        for (uint8_t i = 1; i < TIME_SYNC_MAX_SENDERS; ++i) {
        if (senders_[i].active && senders_[i].sequence < oldestSeq) {
            oldestSeq = senders_[i].sequence;
            oldestIdx = i;
        }
    }
    return oldestIdx;
}
Status extractAdvertTimestamp(const uint8_t* payload, uint16_t payloadLen,
                               uint32_t& timestamp, uint8_t& senderHash) {
    if (payload == nullptr || payloadLen < ADVERT_MIN_SIZE) {
        return ErrorCode::InvalidParameter;
    }
        senderHash = payload[0];
    std::memcpy(&timestamp, &payload[ADVERT_TIMESTAMP_OFFSET], sizeof(timestamp));
        return Status();
}
}
