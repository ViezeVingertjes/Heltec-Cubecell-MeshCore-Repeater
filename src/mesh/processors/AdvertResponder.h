#pragma once

#include <stdint.h>
#include "../../core/PacketDecoder.h"
#include "../PacketDispatcher.h"

class AdvertResponder : public MeshCore::IPacketProcessor {
public:
  AdvertResponder() : lastPayloadHash(0), lastPayloadTime(0), lastResponseTime(0), 
                      pendingResponse(false), responseTime(0) {}

  MeshCore::ProcessResult processPacket(const MeshCore::PacketEvent &event,
                              MeshCore::ProcessingContext &ctx) override;
  const char *getName() const override { return "AdvertResponder"; }
  uint8_t getPriority() const override { return 35; }  // After forwarding, before logging
  
  void loop();
  bool hasPendingResponse() const { return pendingResponse; }

private:
  static constexpr uint32_t ADVERT_RATE_LIMIT_MINUTES = 1;
  static constexpr uint32_t RESPONSE_RATE_LIMIT_MS = ADVERT_RATE_LIMIT_MINUTES * 60 * 1000UL;
  static constexpr uint32_t DEDUP_TIMEOUT_MS = 60000; // 60 seconds for !advert deduplication

  uint32_t lastPayloadHash;
  uint32_t lastPayloadTime;
  uint32_t lastResponseTime;
  bool pendingResponse;
  uint32_t responseTime;
  uint8_t pendingPacket[256]; // Pre-encoded packet
  uint16_t pendingPacketLength;

  static uint32_t hashPayload(const MeshCore::DecodedPacket &packet);
  uint32_t calculateResponseDelay(uint16_t packetLength) const;
  bool buildAdvertPacket(uint8_t *dest, uint16_t &length);
  uint8_t buildAdvertAppData(uint8_t *appData);
};


