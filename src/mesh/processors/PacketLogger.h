#pragma once

#include "../../core/Logger.h"
#include "../PacketDispatcher.h"

namespace MeshCore {

class PacketLogger : public IPacketProcessor {
public:
  PacketLogger() = default;
  ~PacketLogger() override = default;

  ProcessResult processPacket(const PacketEvent &event,
                              ProcessingContext &ctx) override;
  const char *getName() const override { return "PacketLogger"; }
  uint8_t getPriority() const override { return 99; }  // Last - just for debugging
};

} // namespace MeshCore
