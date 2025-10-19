#include "Deduplicator.h"
#include <string.h>

namespace MeshCore {

Deduplicator::Deduplicator() 
    : nextCacheIndex(0), duplicateCount(0) {
    resetCache();
}

void Deduplicator::resetCache() {
    for (size_t i = 0; i < CACHE_SIZE; ++i) {
        cache[i].valid = false;
        cache[i].hash = 0;
        cache[i].timestamp = 0;
    }
    nextCacheIndex = 0;
}

uint32_t Deduplicator::computePacketHash(const DecodedPacket& packet) {
    uint32_t hash = 2166136261u;
    const uint32_t FNV_PRIME = 16777619u;
    
    uint8_t payloadType = static_cast<uint8_t>(packet.payloadType);
    hash ^= payloadType;
    hash *= FNV_PRIME;
    
    hash ^= packet.payloadVersion;
    hash *= FNV_PRIME;
    
    for (uint16_t i = 0; i < packet.payloadLength; ++i) {
        hash ^= packet.payload[i];
        hash *= FNV_PRIME;
    }
    
    if (packet.payloadType == PayloadType::TRACE && packet.routeType == RouteType::DIRECT) {
        hash ^= packet.pathLength;
        hash *= FNV_PRIME;
    }
    
    return hash;
}

uint16_t Deduplicator::extractSourceNode(const DecodedPacket& packet) {
    if (packet.hasTransportCodes) {
        LOG_DEBUG_FMT("Source from transport[0]: %u", packet.transportCodes[0]);
        return packet.transportCodes[0];
    }
    
    if (packet.pathLength >= 2) {
        uint16_t source;
        memcpy(&source, packet.path, 2);
        LOG_DEBUG_FMT("Source from path[0]: %u", source);
        return source;
    }
    
    return 0;
}

bool Deduplicator::isDuplicate(uint32_t hash, uint32_t timestamp) {
    for (size_t i = 0; i < CACHE_SIZE; ++i) {
        if (!cache[i].valid) continue;
        
        if (timestamp - cache[i].timestamp > CACHE_TIMEOUT_MS) {
            cache[i].valid = false;
            continue;
        }
        
        if (cache[i].hash == hash) {
            return true;
        }
    }
    
    return false;
}

void Deduplicator::addToCache(uint32_t hash, uint32_t timestamp) {
    cache[nextCacheIndex].hash = hash;
    cache[nextCacheIndex].timestamp = timestamp;
    cache[nextCacheIndex].valid = true;
    
    nextCacheIndex = (nextCacheIndex + 1) % CACHE_SIZE;
}

void Deduplicator::cleanExpiredEntries(uint32_t currentTime) {
    for (size_t i = 0; i < CACHE_SIZE; ++i) {
        if (cache[i].valid && (currentTime - cache[i].timestamp > CACHE_TIMEOUT_MS)) {
            cache[i].valid = false;
        }
    }
}

ProcessResult Deduplicator::processPacket(const PacketEvent& event, ProcessingContext& ctx) {
    cleanExpiredEntries(event.timestamp);
    
    uint32_t hash = computePacketHash(event.packet);
    
    if (isDuplicate(hash, event.timestamp)) {
        ctx.isDuplicate = true;
        duplicateCount++;
        LOG_DEBUG_FMT("Duplicate detected: hash=0x%08lX", hash);
        return ProcessResult::DROP;
    }
    
    addToCache(hash, event.timestamp);
    
    ctx.sourceNode = extractSourceNode(event.packet);
    
    LOG_DEBUG_FMT("New packet cached: hash=0x%08lX src=%u", hash, ctx.sourceNode);
    
    return ProcessResult::CONTINUE;
}

}

