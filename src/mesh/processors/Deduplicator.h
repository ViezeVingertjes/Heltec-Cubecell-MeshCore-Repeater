#pragma once

#include "../PacketDispatcher.h"
#include "../../core/Logger.h"
#include "../../core/Config.h"

namespace MeshCore {

class Deduplicator : public IPacketProcessor {
public:
    Deduplicator();
    ~Deduplicator() override = default;
    
    ProcessResult processPacket(const PacketEvent& event, ProcessingContext& ctx) override;
    const char* getName() const override { return "Deduplicator"; }
    uint8_t getPriority() const override { return 10; }
    
    void resetCache();
    uint32_t getDuplicateCount() const { return duplicateCount; }
    
    static uint32_t computePacketHash(const DecodedPacket& packet);

private:
    struct PacketHash {
        uint32_t hash;
        uint32_t timestamp;
        bool valid;
    };
    
    static constexpr size_t CACHE_SIZE = Config::Deduplication::CACHE_SIZE;
    static constexpr uint32_t CACHE_TIMEOUT_MS = Config::Deduplication::CACHE_TIMEOUT_MS;
    
    PacketHash cache[CACHE_SIZE];
    size_t nextCacheIndex;
    uint32_t duplicateCount;
    
    bool isDuplicate(uint32_t hash, uint32_t timestamp);
    void addToCache(uint32_t hash, uint32_t timestamp);
    void cleanExpiredEntries(uint32_t currentTime);
    uint16_t extractSourceNode(const DecodedPacket& packet);
};

}

