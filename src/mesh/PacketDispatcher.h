#pragma once

#include "../core/Config.h"
#include "../core/PacketDecoder.h"

namespace MeshCore {

struct PacketEvent {
  const DecodedPacket &packet;
  int16_t rssi;
  int8_t snr;
  uint32_t timestamp;
  uint32_t hash; // Computed hash for deduplication (set by Deduplicator)

  PacketEvent(const DecodedPacket &p, int16_t r, int8_t s, uint32_t t)
      : packet(p), rssi(r), snr(s), timestamp(t), hash(0) {}
};

enum class ProcessResult : uint8_t { CONTINUE, STOP, DROP };

struct ProcessingContext {
  bool isDuplicate = false;
  bool shouldForward = false;
  bool isForUs = false;
  uint8_t hopCount = 0;
  uint16_t sourceNode = 0;
  uint16_t targetNode = 0;

  void reset() {
    isDuplicate = false;
    shouldForward = false;
    isForUs = false;
    hopCount = 0;
    sourceNode = 0;
    targetNode = 0;
  }
};

class IPacketProcessor {
public:
  virtual ~IPacketProcessor() = default;
  virtual ProcessResult processPacket(const PacketEvent &event,
                                      ProcessingContext &ctx) = 0;
  virtual const char *getName() const = 0;
  virtual uint8_t getPriority() const = 0;
};

class PacketDispatcher {
public:
  static PacketDispatcher &getInstance();

  void addProcessor(IPacketProcessor *processor);
  void removeProcessor(IPacketProcessor *processor);
  void dispatchPacket(const PacketEvent &event);

  size_t getProcessorCount() const { return processorCount; }

private:
  PacketDispatcher() : processorCount(0), processors{nullptr} {}

  static constexpr size_t MAX_PROCESSORS = Config::Dispatcher::MAX_PROCESSORS;
  IPacketProcessor *processors[MAX_PROCESSORS];
  size_t processorCount;

  void sortProcessorsByPriority();

  PacketDispatcher(const PacketDispatcher &) = delete;
  PacketDispatcher &operator=(const PacketDispatcher &) = delete;
};

} // namespace MeshCore
