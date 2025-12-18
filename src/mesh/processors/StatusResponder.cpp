#include "StatusResponder.h"

#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include "../../core/HashUtils.h"
#include "../../core/Logger.h"
#include "../../core/NodeConfig.h"
#include "../../core/TimeSync.h"
#include "../../core/Config.h"
#include "../../power/PowerManager.h"
#include "../../radio/LoRaReceiver.h"
#include "../../radio/LoRaTransmitter.h"
#include "../channels/PrivateChannelAnnouncer.h"
#include "../channels/ChannelAnnouncer.h"

using MeshCore::PayloadType;

uint32_t StatusResponder::hashPayload(const MeshCore::DecodedPacket &packet) {
  return HashUtils::fnv1a(packet.payload, packet.payloadLength);
}

MeshCore::ProcessResult
StatusResponder::processPacket(const MeshCore::PacketEvent &event,
                               MeshCore::ProcessingContext &) {
  if (event.packet.payloadType != PayloadType::GRP_TXT) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  uint32_t msgTimestamp = 0;
  char text[ChannelAnnouncer::MAX_MESSAGE_LEN];
  uint8_t privateChannelIndex = 0;
  
  // Only allow !status command in private channels to prevent public abuse
  if (!PrivateChannelAnnouncer::getInstance().decodeMessage(event.packet,
                                                            msgTimestamp, text,
                                                            sizeof(text),
                                                            privateChannelIndex)) {
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

  if (strcmp(content, "!status") != 0) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Rate limiting: only respond once per minute to prevent spam abuse
  uint32_t now = millis();
  if (lastResponseTime != 0 && (now - lastResponseTime) < RESPONSE_RATE_LIMIT_MS) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Deduplicate based on payload hash to prevent responding to
  // repeated/forwarded copies of the same !status
  uint32_t payloadHash = hashPayload(event.packet);
  if (payloadHash == lastPayloadHash && (now - lastPayloadTime) < DEDUP_TIMEOUT_MS) {
    return MeshCore::ProcessResult::CONTINUE;
  }
  lastPayloadHash = payloadHash;
  lastPayloadTime = now;

  // Prepare response message
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  
  // Format uptime info
  uint32_t totalUptime = millis();
  uint32_t sleepTime = PowerManager::getInstance().getTotalSleepTime();
  uint32_t wakeTime = totalUptime > sleepTime ? totalUptime - sleepTime : 0;
  
  // Helper function to format time durations
  auto formatUptime = [](char* dest, size_t len, uint32_t milliseconds) {
    uint32_t totalSeconds = milliseconds / 1000;
    uint32_t seconds = totalSeconds % 60;
    uint32_t totalMinutes = totalSeconds / 60;
    uint32_t minutes = totalMinutes % 60;
    uint32_t totalHours = totalMinutes / 60;
    uint32_t hours = totalHours % 24;
    uint32_t days = totalHours / 24;
    
    if (days > 0) {
      snprintf(dest, len, "%lud %luh", days, hours);
    } else if (hours > 0) {
      snprintf(dest, len, "%luh %lum", hours, minutes);
    } else if (minutes > 0) {
      snprintf(dest, len, "%lum %lus", minutes, seconds);
    } else {
      snprintf(dest, len, "%lus", seconds);
    }
  };
  
  char uptimeText[24];
  char wakeText[24];
  char sleepText[24];
  formatUptime(uptimeText, sizeof(uptimeText), totalUptime);
  formatUptime(wakeText, sizeof(wakeText), wakeTime);
  formatUptime(sleepText, sizeof(sleepText), sleepTime);
  
  // Calculate sleep percentage
  uint8_t sleepPercent = totalUptime > 0 ? (sleepTime * 100) / totalUptime : 0;
  
  // Get packet count
  uint32_t packetCount = LoRaReceiver::getPacketCount();
  
  char message[ChannelAnnouncer::MAX_MESSAGE_LEN];
  snprintf(message, sizeof(message), "%s %02X: U:%s W:%s S:%s (S:%u%%) P:%lu", 
           Config::Identity::NODE_NAME, nodeHash, uptimeText, wakeText, sleepText, sleepPercent, packetCount);

  // Build packet to get ACTUAL encoded length (not estimated)
  // Respond on the same private channel as the request
  uint32_t timestamp = TimeSync::now();
  bool buildSuccess = PrivateChannelAnnouncer::getInstance().buildPacket(message, pendingPacket, 
                                                                          pendingPacketLength, timestamp,
                                                                          privateChannelIndex);
  
  if (!buildSuccess) {
    LOG_WARN("Failed to build status response packet");
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Calculate jitter based on ACTUAL packet length
  uint32_t jitter = calculateResponseDelay(pendingPacketLength);
  pendingResponse = true;
  responseTime = millis() + jitter;
  
  LOG_INFO_FMT("Queued !status response (%u bytes) with %lu ms jitter on private channel %d", 
               pendingPacketLength, jitter, privateChannelIndex);
  return MeshCore::ProcessResult::CONTINUE;
}

void StatusResponder::loop() {
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

uint32_t StatusResponder::calculateResponseDelay(uint16_t packetLength) const {
  // Use airtime-based slotted jitter similar to packet forwarding
  // Calculate airtime based on ACTUAL packet length (not estimated)
  uint32_t airtime = LoRaTransmitter::estimateAirtime(packetLength);
  
  // Create larger slot window for status responses since multiple nodes respond
  // Use same factor as forwarding (2.0) but more slots (0-9 instead of 0-5)
  uint32_t slotTime = static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
  uint32_t randomSlot = random(0, 10); // 0-9 slots for more distribution
  
  // Add deterministic offset based on node hash for consistent spread
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  uint32_t hashSlot = nodeHash % 10; // 0-9 based on node hash
  
  return (randomSlot + hashSlot) * slotTime;
}

