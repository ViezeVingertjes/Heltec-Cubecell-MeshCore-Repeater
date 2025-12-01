#include "PingResponder.h"
#include "../../core/Logger.h"
#include "../../core/NodeConfig.h"
#include "../../power/BatteryUtils.h"
#include "../../power/BatteryEstimator.h"
#include "../../core/TimeSync.h"
#include <Arduino.h>
#include <string.h>

using MeshCore::PayloadType;

uint32_t PingResponder::hashPayload(const MeshCore::DecodedPacket &packet) {
  uint32_t hash = 2166136261u;
  for (uint16_t i = 0; i < packet.payloadLength; ++i) {
    hash ^= packet.payload[i];
    hash *= 16777619u;
  }
  return hash;
}

MeshCore::ProcessResult
PingResponder::processPacket(const MeshCore::PacketEvent &event,
                             MeshCore::ProcessingContext &) {
  if (event.packet.payloadType != PayloadType::GRP_TXT) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  uint32_t msgTimestamp = 0;
  char text[PublicChannelAnnouncer::MAX_MESSAGE_LEN];
  if (!PublicChannelAnnouncer::getInstance().decodeMessage(event.packet,
                                                           msgTimestamp, text,
                                                           sizeof(text))) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  const char *content = text;
  const char *colon = strchr(text, ':');
  if (colon != nullptr) {
    content = colon + 1;
    while (*content == ' ') {
      ++content;
    }
  }

  if (strcmp(content, "!ping") != 0) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Rate limiting: only respond once per 15 minutes to prevent spam abuse
  uint32_t now = millis();
  if (lastResponseTime != 0 && (now - lastResponseTime) < RESPONSE_RATE_LIMIT_MS) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Deduplicate based on payload hash with 60-second timeout
  // This prevents responding to repeated/forwarded copies of the same !ping
  // but allows responding to new !ping commands after the timeout
  uint32_t payloadHash = hashPayload(event.packet);
  if (payloadHash == lastPayloadHash && (now - lastPayloadTime) < 60000) {
    return MeshCore::ProcessResult::CONTINUE;
  }
  lastPayloadHash = payloadHash;
  lastPayloadTime = now;

  // Prepare response message
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  uint16_t batteryMv = getBatteryVoltage();
  uint8_t percent = BatteryUtils::estimatePercent(batteryMv);
  int32_t hoursRemaining = -1;
  BatteryEstimator::estimateHours(hoursRemaining);
  char message[PublicChannelAnnouncer::MAX_MESSAGE_LEN];
  BatteryUtils::formatStatus(message, sizeof(message), nodeHash, 
                             batteryMv, percent, false, hoursRemaining);

  // Build packet to get ACTUAL encoded length (not estimated)
  uint32_t timestamp = TimeSync::now();
  if (!PublicChannelAnnouncer::getInstance().buildPacket(message, pendingPacket, 
                                                         pendingPacketLength, timestamp)) {
    LOG_WARN("Failed to build pong packet");
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Calculate jitter based on ACTUAL packet length
  uint32_t jitter = calculateResponseDelay(pendingPacketLength);
  pendingResponse = true;
  responseTime = millis() + jitter;
  
  LOG_INFO_FMT("Queued !ping response (%u bytes) with %lu ms jitter", 
               pendingPacketLength, jitter);
  return MeshCore::ProcessResult::CONTINUE;
}

void PingResponder::loop() {
  if (!pendingResponse) {
    return;
  }
  
  if (millis() >= responseTime) {
    // Check if transmitter is available
    auto &tx = LoRaTransmitter::getInstance();
    if (tx.isTransmitting()) {
      // Transmitter busy, retry after one airtime slot
      uint32_t airtime = LoRaTransmitter::estimateAirtime(pendingPacketLength);
      uint32_t retryDelay = static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
      responseTime = millis() + retryDelay;
      return;
    }
    
    // Try to transmit pre-built packet
    if (!tx.transmit(pendingPacket, pendingPacketLength)) {
      // Failed (likely duty cycle / airtime budget)
      // Retry after one airtime slot
      uint32_t airtime = LoRaTransmitter::estimateAirtime(pendingPacketLength);
      uint32_t retryDelay = static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
      responseTime = millis() + retryDelay;
      return;
    }
    
    // Successfully sent - update rate limit timestamp
    lastResponseTime = millis();
    pendingResponse = false;
  }
}

uint32_t PingResponder::calculateResponseDelay(uint16_t packetLength) const {
  // Use airtime-based slotted jitter similar to packet forwarding
  // Calculate airtime based on ACTUAL packet length (not estimated)
  uint32_t airtime = LoRaTransmitter::estimateAirtime(packetLength);
  
  // Create larger slot window for ping responses since multiple nodes respond
  // Use same factor as forwarding (2.0) but more slots (0-9 instead of 0-5)
  uint32_t slotTime = static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
  uint32_t randomSlot = random(0, 10); // 0-9 slots for more distribution
  
  // Add deterministic offset based on node hash for consistent spread
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  uint32_t hashSlot = nodeHash % 10; // 0-9 based on node hash
  
  return (randomSlot + hashSlot) * slotTime;
}

