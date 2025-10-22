#pragma once

#include "../../core/Config.h"
#include "../../core/Logger.h"
#include "../../core/containers/CircularBuffer.h"
#include "../PacketDispatcher.h"

namespace MeshCore {

/**
 * Deduplicator prevents forwarding the same packet multiple times.
 * Uses a circular buffer cache with time-based expiration.
 */
class Deduplicator : public IPacketProcessor {
public:
  Deduplicator();
  ~Deduplicator() override = default;

  ProcessResult processPacket(const PacketEvent &event,
                              ProcessingContext &ctx) override;
  const char *getName() const override { return "Deduplicator"; }
  uint8_t getPriority() const override { return 10; }

  void resetCache();
  uint32_t getDuplicateCount() const { return duplicateCount; }

  static uint32_t computePacketHash(const DecodedPacket &packet);

private:
  struct PacketHash {
    uint32_t hash;
    uint32_t timestamp;
    bool valid;

    PacketHash() : hash(0), timestamp(0), valid(false) {}
  };

  static constexpr size_t CACHE_SIZE = Config::Deduplication::CACHE_SIZE;
  static constexpr uint32_t CACHE_TIMEOUT_MS =
      Config::Deduplication::CACHE_TIMEOUT_MS;

  CircularBuffer<PacketHash, CACHE_SIZE> cache;
  uint32_t duplicateCount;

  bool isDuplicate(uint32_t hash, uint32_t timestamp);
  void addToCache(uint32_t hash, uint32_t timestamp);
  void cleanExpiredEntries(uint32_t currentTime);
  uint16_t extractSourceNode(const DecodedPacket &packet);
};

} // namespace MeshCore
