#pragma once
#include <cstdint>
#include <cstddef>
#include "core/config.h"
#include "core/result.h"
namespace MiniCore {
using Config::ADVERT_PUBKEY_SIZE;
using Config::ADVERT_TIMESTAMP_OFFSET;
using Config::ADVERT_TIMESTAMP_SIZE;
using Config::ADVERT_SIGNATURE_SIZE;
using Config::ADVERT_MIN_SIZE;
using Config::TIME_SYNC_MAX_SENDERS;
using Config::TIME_SYNC_MIN_CONSENSUS;
class TimeSynchronizer {
public:
    TimeSynchronizer();
    void addSample(uint8_t senderHash, uint32_t receivedTimestamp, uint32_t localTime);
    [[nodiscard]] int32_t getOffset() const;
    [[nodiscard]] bool hasConsensus() const;
    [[nodiscard]] uint32_t adjustedTime(uint32_t localTime) const;
    [[nodiscard]] uint8_t uniqueSenderCount() const;
    void reset();
private:
    struct SenderOffset {
        uint8_t senderHash;
        int32_t offset;
        uint16_t sequence;
        bool active;
    };
    SenderOffset senders_[TIME_SYNC_MAX_SENDERS];
    uint8_t senderCount_;
    uint16_t sequenceCounter_;
    void addOrUpdateSample(uint8_t senderHash, int32_t offset);
    [[nodiscard]] int32_t calculateMedianOffset() const;
    [[nodiscard]] int8_t findSender(uint8_t senderHash) const;
    [[nodiscard]] uint8_t findOldestSlot() const;
};
Status extractAdvertTimestamp(const uint8_t* payload, uint16_t payloadLen,
                               uint32_t& timestamp, uint8_t& senderHash);
}
