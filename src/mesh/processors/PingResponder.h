#pragma once

#include "../../core/PacketDecoder.h"
#include "../PacketDispatcher.h"
#include "../../mesh/PublicChannelAnnouncer.h"
#include "../../radio/LoRaTransmitter.h"

class PingResponder : public MeshCore::IPacketProcessor {
public:
  PingResponder() : lastPayloadHash(0), lastPayloadTime(0), lastResponseTime(0), 
                    pendingResponse(false), responseTime(0) {}

  MeshCore::ProcessResult processPacket(const MeshCore::PacketEvent &event,
                              MeshCore::ProcessingContext &ctx) override;
  const char *getName() const override { return "PingResponder"; }
  uint8_t getPriority() const override { return 50; }
  
  void loop();
  bool hasPendingResponse() const { return pendingResponse; }

private:
  static constexpr uint32_t PING_RATE_LIMIT_MINUTES = 1;
  static constexpr uint32_t RESPONSE_RATE_LIMIT_MS = PING_RATE_LIMIT_MINUTES * 60 * 1000UL;
  static constexpr uint32_t DEDUP_TIMEOUT_MS = 60000; // 60 seconds for !ping deduplication

  uint32_t lastPayloadHash;
  uint32_t lastPayloadTime;
  uint32_t lastResponseTime;
  bool pendingResponse;
  uint32_t responseTime;
  uint8_t pendingPacket[256]; // Pre-encoded packet
  uint16_t pendingPacketLength;

  static uint32_t hashPayload(const MeshCore::DecodedPacket &packet);
  uint32_t calculateResponseDelay(uint16_t packetLength) const;
};

