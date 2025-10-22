#include "Deduplicator.h"
#include "../../core/PacketDecoder.h"
#include <string.h>

namespace MeshCore {

Deduplicator::Deduplicator() : duplicateCount(0) { resetCache(); }

void Deduplicator::resetCache() {
  cache.clear();
  duplicateCount = 0;
}

uint32_t Deduplicator::computePacketHash(const DecodedPacket &packet) {
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

  if (packet.payloadType == PayloadType::TRACE &&
      packet.routeType == RouteType::DIRECT) {
    hash ^= packet.pathLength;
    hash *= FNV_PRIME;
  }

  return hash;
}

uint16_t Deduplicator::extractSourceNode(const DecodedPacket &packet) {
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
  bool found = false;

  cache.forEach([&](PacketHash &entry, size_t index) {
    if (!entry.valid) {
      return true; // Continue iteration
    }

    // Check if expired
    if (timestamp - entry.timestamp > CACHE_TIMEOUT_MS) {
      entry.valid = false;
      return true; // Continue
    }

    // Check for match
    if (entry.hash == hash) {
      found = true;
      return false; // Stop iteration
    }

    return true; // Continue
  });

  return found;
}

void Deduplicator::addToCache(uint32_t hash, uint32_t timestamp) {
  PacketHash newEntry;
  newEntry.hash = hash;
  newEntry.timestamp = timestamp;
  newEntry.valid = true;

  cache.push(newEntry);
}

void Deduplicator::cleanExpiredEntries(uint32_t currentTime) {
  cache.forEach([&](PacketHash &entry, size_t index) {
    if (entry.valid && (currentTime - entry.timestamp > CACHE_TIMEOUT_MS)) {
      entry.valid = false;
    }
    return true; // Continue iteration
  });
}

ProcessResult Deduplicator::processPacket(const PacketEvent &event,
                                          ProcessingContext &ctx) {
  cleanExpiredEntries(event.timestamp);

  uint32_t hash = computePacketHash(event.packet);

  LOG_INFO_FMT("Packet hash: 0x%08lX (%s/%s, payload=%d bytes)", hash,
               PacketDecoder::routeTypeToString(event.packet.routeType),
               PacketDecoder::payloadTypeToString(event.packet.payloadType),
               event.packet.payloadLength);

  if (isDuplicate(hash, event.timestamp)) {
    ctx.isDuplicate = true;
    duplicateCount++;
    LOG_INFO_FMT(">>> DUPLICATE DETECTED: hash=0x%08lX <<<", hash);
    return ProcessResult::DROP;
  }

  addToCache(hash, event.timestamp);

  ctx.sourceNode = extractSourceNode(event.packet);

  // Store hash in event for other processors to use
  const_cast<PacketEvent &>(event).hash = hash;

  LOG_DEBUG_FMT("New packet cached: hash=0x%08lX src=%u", hash, ctx.sourceNode);

  return ProcessResult::CONTINUE;
}

} // namespace MeshCore
