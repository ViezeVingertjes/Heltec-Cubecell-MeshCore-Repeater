#pragma once

#include <stdint.h>
#include "../../core/PacketDecoder.h"
#include "../PacketDispatcher.h"

/**
 * CommandHandler - Unified handler for all private channel text commands
 * 
 * Handles: !status, !clear, !advert, !location
 * Consolidates duplicate code from Status and AdvertResponders to save flash space.
 */
class CommandHandler : public MeshCore::IPacketProcessor {
public:
  CommandHandler() : lastPayloadHash(0), lastPayloadTime(0), lastResponseTime(0), 
                     pendingResponse(false), responseTime(0) {}

  MeshCore::ProcessResult processPacket(const MeshCore::PacketEvent &event,
                              MeshCore::ProcessingContext &ctx) override;
  const char *getName() const override { return "CommandHandler"; }
  uint8_t getPriority() const override { return 35; }  // After forwarding, before logging
  
  void loop();
  bool hasPendingResponse() const { return pendingResponse; }

private:
  static constexpr uint32_t RESPONSE_RATE_LIMIT_MS = 60000; // 1 minute
  static constexpr uint32_t DEDUP_TIMEOUT_MS = 60000; // 60 seconds

  uint32_t lastPayloadHash;
  uint32_t lastPayloadTime;
  uint32_t lastResponseTime;
  bool pendingResponse;
  uint32_t responseTime;
  uint8_t pendingPacket[256]; // Pre-encoded packet
  uint16_t pendingPacketLength;

  static uint32_t hashPayload(const MeshCore::DecodedPacket &packet);
  uint32_t calculateResponseDelay(uint16_t packetLength) const;
  
  // Command handlers
  bool handleStatusCommand(const char *args, uint8_t privateChannelIndex);
  bool handleAdvertCommand(uint8_t privateChannelIndex);
  bool handleLocationCommand(const char *args, uint8_t privateChannelIndex);
  bool handleNeighborsCommand(uint8_t privateChannelIndex);
  bool handleHelpCommand(uint8_t privateChannelIndex);
  
  // Helper for advert
  bool buildAdvertPacket(uint8_t *dest, uint16_t &length);
  uint8_t buildAdvertAppData(uint8_t *appData);
};

