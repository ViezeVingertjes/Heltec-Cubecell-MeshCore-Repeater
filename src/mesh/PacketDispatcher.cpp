#include "PacketDispatcher.h"
#include "../core/Logger.h"

namespace MeshCore {

PacketDispatcher &PacketDispatcher::getInstance() {
  static PacketDispatcher instance;
  return instance;
}

void PacketDispatcher::addProcessor(IPacketProcessor *processor) {
  if (processor == nullptr || processorCount >= MAX_PROCESSORS)
    return;

  for (size_t i = 0; i < processorCount; ++i) {
    if (processors[i] == processor)
      return;
  }

  processors[processorCount++] = processor;
  sortProcessorsByPriority();
}

void PacketDispatcher::removeProcessor(IPacketProcessor *processor) {
  for (size_t i = 0; i < processorCount; ++i) {
    if (processors[i] == processor) {
      for (size_t j = i; j < processorCount - 1; ++j) {
        processors[j] = processors[j + 1];
      }
      processorCount--;
      break;
    }
  }
}

void PacketDispatcher::sortProcessorsByPriority() {
  for (size_t i = 1; i < processorCount; ++i) {
    IPacketProcessor *key = processors[i];
    size_t j = i;

    while (j > 0 && processors[j - 1]->getPriority() > key->getPriority()) {
      processors[j] = processors[j - 1];
      j--;
    }
    processors[j] = key;
  }
}

void PacketDispatcher::dispatchPacket(const PacketEvent &event) {
  ProcessingContext ctx;

  for (size_t i = 0; i < processorCount; ++i) {
    if (processors[i] != nullptr) {
      ProcessResult result = processors[i]->processPacket(event, ctx);

      if (result == ProcessResult::DROP) {
        LOG_DEBUG_FMT("Packet dropped by processor: %s",
                      processors[i]->getName());
        return;
      } else if (result == ProcessResult::STOP) {
        LOG_DEBUG_FMT("Pipeline stopped by processor: %s",
                      processors[i]->getName());
        return;
      }
    }
  }
}

} // namespace MeshCore
