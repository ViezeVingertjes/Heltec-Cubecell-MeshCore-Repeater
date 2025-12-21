#pragma once

#include "../../core/PacketDecoder.h"
#include "../PacketDispatcher.h"

/**
 * NeighborMonitor - Tracks nearby repeaters from ADVERT packets
 * 
 * Passively monitors network to maintain neighbor table.
 */
class NeighborMonitor : public MeshCore::IPacketProcessor {
public:
  NeighborMonitor() = default;

  MeshCore::ProcessResult processPacket(const MeshCore::PacketEvent &event,
                                       MeshCore::ProcessingContext &ctx) override;
  const char *getName() const override { return "NeighborMonitor"; }
  uint8_t getPriority() const override { return 50; }  // Low priority, just observing
};

