#include "DiscoveryResponder.h"
#include "../../core/CryptoIdentity.h"
#include "../../core/NodeConfig.h"
#include "../../radio/LoRaTransmitter.h"
#include <Arduino.h>
#include <string.h>

namespace MeshCore {

// CONTROL packet sub-types
constexpr uint8_t CONTROL_DISCOVER_REQ = 0x80;
constexpr uint8_t CONTROL_DISCOVER_RESP = 0x90;

// DISCOVER_REQ flags
constexpr uint8_t DISCOVER_PREFIX_ONLY = 0x01;

// Advert types (used as bit positions, not masks)
// These values get shifted: (1 << ADV_TYPE_X) to create the filter bitmap
constexpr uint8_t ADV_TYPE_CHAT = 1;
constexpr uint8_t ADV_TYPE_REPEATER = 2;
constexpr uint8_t ADV_TYPE_ROOM = 3;
constexpr uint8_t ADV_TYPE_SENSOR = 4;

ProcessResult DiscoveryResponder::processPacket(const PacketEvent &event,
                                                ProcessingContext &ctx) {
  // Debug: Log all CONTROL packets we receive
  if (event.packet.payloadType == PayloadType::CONTROL) {
    LOG_INFO_FMT("RX CONTROL packet: route=%d, path_len=%d, payload_len=%d",
                 static_cast<int>(event.packet.routeType), 
                 event.packet.pathLength,
                 event.packet.payloadLength);
    if (event.packet.payloadLength > 0) {
      LOG_INFO_FMT("  flags=0x%02X, subtype=0x%02X", 
                   event.packet.payload[0],
                   event.packet.payload[0] & 0xF0);
    }
  }

  // Only handle zero-hop DIRECT CONTROL packets (matching official MeshCore)
  // Zero-hop means: DIRECT routing with path_len = 0 (broadcast to neighbors only)
  bool isDirect = (event.packet.routeType == RouteType::DIRECT ||
                   event.packet.routeType == RouteType::TRANSPORT_DIRECT);
  bool isZeroHop = (event.packet.pathLength == 0);
  
  if (!isDirect || !isZeroHop) {
    if (event.packet.payloadType == PayloadType::CONTROL) {
      LOG_DEBUG_FMT("CONTROL not zero-hop DIRECT: isDirect=%d, isZeroHop=%d",
                    isDirect, isZeroHop);
    }
    return ProcessResult::CONTINUE;
  }

  // Only handle CONTROL packets
  if (event.packet.payloadType != PayloadType::CONTROL) {
    return ProcessResult::CONTINUE;
  }

  // Need at least flags byte
  if (event.packet.payloadLength < 1) {
    LOG_WARN("CONTROL packet too short");
    return ProcessResult::CONTINUE;
  }

  uint8_t flags = event.packet.payload[0];
  uint8_t subType = flags & 0xF0;

  // Only handle DISCOVER_REQ (0x80 in upper 4 bits)
  if (subType != CONTROL_DISCOVER_REQ) {
    LOG_DEBUG_FMT("CONTROL packet not DISCOVER_REQ: subtype=0x%02X", subType);
    return ProcessResult::CONTINUE;
  }
  
  LOG_INFO("*** Received DISCOVER_REQ ***");

  // DISCOVER_REQ format: flags(1) + type_filter(1) + tag(4) + since(4, optional)
  if (event.packet.payloadLength < 6) {
    LOG_WARN("DISCOVER_REQ too short");
    return ProcessResult::CONTINUE;
  }

  uint8_t typeFilter = event.packet.payload[1];
  uint32_t tag;
  memcpy(&tag, &event.packet.payload[2], 4);

  LOG_INFO_FMT("DISCOVER_REQ: typeFilter=0x%02X, tag=0x%08lX", typeFilter, tag);

  // Check if this repeater matches the type filter
  // Official MeshCore uses: (filter & (1 << ADV_TYPE_REPEATER))
  // ADV_TYPE_REPEATER=2, so (1<<2) = 0x04
  uint8_t repeaterMask = (1 << ADV_TYPE_REPEATER);
  if ((typeFilter & repeaterMask) == 0) {
    LOG_INFO_FMT("Type filter 0x%02X doesn't include repeaters (mask=0x%02X), ignoring",
                 typeFilter, repeaterMask);
    return ProcessResult::CONTINUE;
  }
  
  LOG_INFO("Type filter matches! Preparing DISCOVER_RESP...");

  // Rate limiting: only respond once per minute
  uint32_t now = millis();
  if (lastResponseTime != 0 && (now - lastResponseTime) < RESPONSE_RATE_LIMIT_MS) {
    LOG_DEBUG("Rate limited, not responding to discovery");
    return ProcessResult::CONTINUE;
  }

  // Deduplicate by tag to prevent responding to repeated/forwarded requests
  if (tag == lastRequestTag && (now - lastRequestTime) < DEDUP_TIMEOUT_MS) {
    LOG_DEBUG_FMT("Duplicate DISCOVER_REQ tag=0x%08lX, ignoring", tag);
    return ProcessResult::CONTINUE;
  }
  lastRequestTag = tag;
  lastRequestTime = now;

  // Build discovery response
  if (!buildDiscoveryResponse(event.packet.payload, event.packet.payloadLength,
                              event.snr, pendingPacket, pendingPacketLength)) {
    LOG_WARN("Failed to build DISCOVER_RESP");
    return ProcessResult::CONTINUE;
  }

  // Calculate jitter-based delay
  uint32_t jitter = calculateResponseDelay(pendingPacketLength);
  pendingResponse = true;
  responseTime = millis() + jitter;

  LOG_INFO_FMT("Queued DISCOVER_RESP (%u bytes) with %lu ms jitter, tag=0x%08lX",
               pendingPacketLength, jitter, tag);

  return ProcessResult::CONTINUE;
}

void DiscoveryResponder::loop() {
  if (!pendingResponse) {
    return;
  }

  if (millis() >= responseTime) {
    // Check if transmitter is available
    auto &tx = LoRaTransmitter::getInstance();
    if (tx.isTransmitting()) {
      // Transmitter busy, retry after one airtime slot
      uint32_t airtime = LoRaTransmitter::estimateAirtime(pendingPacketLength);
      uint32_t retryDelay =
          static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
      responseTime = millis() + retryDelay;
      return;
    }

    // Try to transmit
    if (!tx.transmit(pendingPacket, pendingPacketLength)) {
      // Failed (likely duty cycle / airtime budget)
      uint32_t airtime = LoRaTransmitter::estimateAirtime(pendingPacketLength);
      uint32_t retryDelay =
          static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
      responseTime = millis() + retryDelay;
      return;
    }

    // Successfully sent
    lastResponseTime = millis();
    pendingResponse = false;
    LOG_INFO("Transmitted DISCOVER_RESP");
  }
}

bool DiscoveryResponder::buildDiscoveryResponse(const uint8_t *requestPayload,
                                                uint16_t requestLength,
                                                int8_t requestSnr,
                                                uint8_t *dest,
                                                uint16_t &length) {
  // Extract request details
  uint8_t requestFlags = requestPayload[0];
  bool prefixOnly = (requestFlags & DISCOVER_PREFIX_ONLY) != 0;
  
  uint32_t tag;
  memcpy(&tag, &requestPayload[2], 4);

  // Build DISCOVER_RESP packet
  DecodedPacket packet;
  memset(&packet, 0, sizeof(packet));

  // DISCOVER_RESP is sent as DIRECT with zero-hop (path_len = 0)
  packet.routeType = RouteType::DIRECT;
  packet.payloadType = PayloadType::CONTROL;
  packet.payloadVersion = 0;
  packet.hasTransportCodes = false;
  packet.pathLength = 0; // Zero-hop means broadcast to immediate neighbors

  // Build header byte
  packet.header = (static_cast<uint8_t>(packet.routeType) & PH_ROUTE_MASK) |
                  ((static_cast<uint8_t>(packet.payloadType) & PH_TYPE_MASK)
                   << PH_TYPE_SHIFT) |
                  ((packet.payloadVersion & PH_VER_MASK) << PH_VER_SHIFT);

  // Build DISCOVER_RESP payload
  // Format: flags(1) + snr(1) + tag(4) + pubkey(8 or 32)
  uint16_t payloadIdx = 0;

  // Flags: 0x90 (DISCOVER_RESP) | node_type (lower 4 bits)
  // Node type uses value directly (not shifted): ADV_TYPE_REPEATER = 2
  uint8_t respFlags = CONTROL_DISCOVER_RESP | ADV_TYPE_REPEATER; // 0x92
  packet.payload[payloadIdx++] = respFlags;

  // SNR (signed byte, already in 0.25 dB units)
  packet.payload[payloadIdx++] = static_cast<uint8_t>(requestSnr);

  // Tag (reflected from request)
  memcpy(&packet.payload[payloadIdx], &tag, 4);
  payloadIdx += 4;

  // Public key (8-byte prefix or full 32 bytes based on request)
  const uint8_t *publicKey = CryptoIdentity::getInstance().getPublicKey();
  uint8_t keyLen = prefixOnly ? 8 : 32;
  memcpy(&packet.payload[payloadIdx], publicKey, keyLen);
  payloadIdx += keyLen;

  packet.payloadLength = payloadIdx;

  // Encode packet
  length = PacketDecoder::encode(packet, dest, 256);
  if (length == 0) {
    LOG_ERROR("Failed to encode DISCOVER_RESP");
    return false;
  }

  LOG_INFO_FMT("Built DISCOVER_RESP: %d bytes, tag=0x%08lX, snr=%d, keyLen=%d",
               length, tag, requestSnr, keyLen);
  return true;
}

uint32_t DiscoveryResponder::calculateResponseDelay(uint16_t packetLength) const {
  // Use airtime-based slotted jitter similar to status responder
  uint32_t airtime = LoRaTransmitter::estimateAirtime(packetLength);
  uint32_t slotTime =
      static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
  uint32_t randomSlot = random(0, 10); // 0-9 slots

  // Add deterministic offset based on node hash for consistent spread
  uint8_t nodeHash = NodeConfig::getInstance().getNodeHash();
  uint32_t hashSlot = nodeHash % 10; // 0-9 based on node hash

  return (randomSlot + hashSlot) * slotTime;
}

} // namespace MeshCore

