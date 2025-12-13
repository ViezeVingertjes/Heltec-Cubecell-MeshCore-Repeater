#pragma once

#include "../../core/Config.h"
#include "../../core/Logger.h"
#include "../../core/PacketDecoder.h"
#include "../PacketDispatcher.h"

namespace MeshCore {

/**
 * DiscoveryResponder handles CONTROL packets with DISCOVER_REQ sub-type.
 * Responds with DISCOVER_RESP to make this repeater visible to network
 * mapping tools and MeshCore clients.
 * 
 * Protocol:
 * - DISCOVER_REQ: flags(0x80+), type_filter, tag(4), since(4, optional)
 * - DISCOVER_RESP: flags(0x90 | node_type), snr, tag(4), pubkey(8 or 32)
 */
class DiscoveryResponder : public IPacketProcessor {
public:
  DiscoveryResponder()
      : lastResponseTime(0), lastRequestTag(0), lastRequestTime(0) {}
  ~DiscoveryResponder() override = default;

  ProcessResult processPacket(const PacketEvent &event,
                              ProcessingContext &ctx) override;
  const char *getName() const override { return "DiscoveryResponder"; }
  uint8_t getPriority() const override { return 36; }

  void loop();
  bool hasPendingResponse() const { return pendingResponse; }

private:
  static constexpr uint32_t RESPONSE_RATE_LIMIT_MS = 60000; // 1 minute
  static constexpr uint32_t DEDUP_TIMEOUT_MS = 30000;       // 30 seconds

  // Rate limiting
  uint32_t lastResponseTime;
  uint32_t lastRequestTag;
  uint32_t lastRequestTime;

  // Pending response
  bool pendingResponse = false;
  uint32_t responseTime = 0;
  uint8_t pendingPacket[256];
  uint16_t pendingPacketLength = 0;

  bool buildDiscoveryResponse(const uint8_t *requestPayload, 
                             uint16_t requestLength,
                             int8_t requestSnr,
                             uint8_t *dest, 
                             uint16_t &length);
  uint32_t calculateResponseDelay(uint16_t packetLength) const;
};

} // namespace MeshCore

