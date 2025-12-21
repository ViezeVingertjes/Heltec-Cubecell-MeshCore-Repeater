#include "NeighborMonitor.h"
#include "../NeighborTracker.h"
#include "../../core/Logger.h"
#include "../../core/Config.h"

using MeshCore::PayloadType;

MeshCore::ProcessResult
NeighborMonitor::processPacket(const MeshCore::PacketEvent &event,
                               MeshCore::ProcessingContext &) {
  // Only process ADVERT packets
  if (event.packet.payloadType != PayloadType::ADVERT) {
    return MeshCore::ProcessResult::CONTINUE;
  }
  
  // ADVERT payload format: public_key(32) + timestamp(4) + signature(64) + appdata(variable)
  // We need at least 1 byte for the node hash
  if (event.packet.payloadLength < 1) {
    return MeshCore::ProcessResult::CONTINUE;
  }
  
  // Extract first byte of public key as node hash (matches MeshCore node identification)
  uint8_t nodeHash = event.packet.payload[0];
  
  // Convert SNR from 0.25 dB units to dB
  int8_t snrDb = event.snr / 4;
  
  // Update neighbor tracker
  NeighborTracker::getInstance().updateNeighbor(nodeHash, snrDb);
  
  return MeshCore::ProcessResult::CONTINUE;
}

