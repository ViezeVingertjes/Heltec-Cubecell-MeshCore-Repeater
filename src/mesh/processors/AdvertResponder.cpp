#include "AdvertResponder.h"

#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include "../../core/HashUtils.h"
#include "../../core/Logger.h"
#include "../../core/NodeConfig.h"
#include "../../core/TimeSync.h"
#include "../../core/Config.h"
#include "../../core/CryptoIdentity.h"
#include "../../radio/LoRaTransmitter.h"
#include "../channels/PrivateChannelAnnouncer.h"
#include "../channels/PublicChannelAnnouncer.h"
#include "../../lib/ed25519/ed_25519.h"

using MeshCore::PayloadType;
using MeshCore::RouteType;

uint32_t AdvertResponder::hashPayload(const MeshCore::DecodedPacket &packet) {
  return HashUtils::fnv1a(packet.payload, packet.payloadLength);
}

MeshCore::ProcessResult
AdvertResponder::processPacket(const MeshCore::PacketEvent &event,
                               MeshCore::ProcessingContext &) {
  if (event.packet.payloadType != PayloadType::GRP_TXT) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  uint32_t msgTimestamp = 0;
  char text[PublicChannelAnnouncer::MAX_MESSAGE_LEN];
  uint8_t privateChannelIndex = 0;
  
  // Only allow !advert command in private channels to prevent public abuse
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

  if (strcmp(content, "!advert") != 0) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Rate limiting: only respond once per minute to prevent spam abuse
  uint32_t now = millis();
  if (lastResponseTime != 0 && (now - lastResponseTime) < RESPONSE_RATE_LIMIT_MS) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Deduplicate based on payload hash to prevent responding to
  // repeated/forwarded copies of the same !advert
  uint32_t payloadHash = hashPayload(event.packet);
  if (payloadHash == lastPayloadHash && (now - lastPayloadTime) < DEDUP_TIMEOUT_MS) {
    return MeshCore::ProcessResult::CONTINUE;
  }
  lastPayloadHash = payloadHash;
  lastPayloadTime = now;

  // Build advert packet
  if (!buildAdvertPacket(pendingPacket, pendingPacketLength)) {
    LOG_WARN("Failed to build advert packet");
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Calculate jitter based on ACTUAL packet length
  uint32_t jitter = calculateResponseDelay(pendingPacketLength);
  pendingResponse = true;
  responseTime = millis() + jitter;
  
  LOG_INFO_FMT("Queued !advert response (%u bytes) with %lu ms jitter", 
               pendingPacketLength, jitter);
  return MeshCore::ProcessResult::CONTINUE;
}

void AdvertResponder::loop() {
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

uint32_t AdvertResponder::calculateResponseDelay(uint16_t packetLength) const {
  // Use airtime-based slotted jitter similar to packet forwarding
  // Calculate airtime based on ACTUAL packet length (not estimated)
  uint32_t airtime = LoRaTransmitter::estimateAirtime(packetLength);
  
  // Create larger slot window for advert responses since multiple nodes respond
  // Use same factor as forwarding (2.0) but more slots (0-9 instead of 0-5)
  uint32_t slotTime = static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
  uint32_t randomSlot = random(0, 10); // 0-9 slots for more distribution
  
  // Add deterministic offset based on node hash for consistent spread
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  uint32_t hashSlot = nodeHash % 10; // 0-9 based on node hash
  
  return (randomSlot + hashSlot) * slotTime;
}

uint8_t AdvertResponder::buildAdvertAppData(uint8_t *appData) {
  // Build appdata according to MeshCore format
  uint8_t flags = static_cast<uint8_t>(MeshCore::AdvertType::REPEATER);
  
  // Add location flag if configured
  if (Config::Identity::HAS_LOCATION) {
    flags |= MeshCore::ADV_LATLON_MASK;
  }
  
  // Add name flag
  flags |= MeshCore::ADV_NAME_MASK;
  
  appData[0] = flags;
  int idx = 1;
  
  // Add location if configured (must come before name per MeshCore spec)
  if (Config::Identity::HAS_LOCATION) {
    memcpy(&appData[idx], &Config::Identity::LOCATION_LATITUDE, 4);
    idx += 4;
    memcpy(&appData[idx], &Config::Identity::LOCATION_LONGITUDE, 4);
    idx += 4;
  }
  
  // Format name with node hash to match chat name format (e.g., "VieZe Rogue 4A")
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  char nameBuffer[MeshCore::MAX_ADVERT_DATA_SIZE];
  snprintf(nameBuffer, sizeof(nameBuffer), "%s %02X", 
           Config::Identity::NODE_NAME, nodeHash);
  
  // Copy formatted name to appData
  const char *name = nameBuffer;
  while (*name && idx < MeshCore::MAX_ADVERT_DATA_SIZE) {
    appData[idx++] = *name++;
  }
  
  return idx;
}

bool AdvertResponder::buildAdvertPacket(uint8_t *dest, uint16_t &length) {
  // Build advert packet according to MeshCore specification
  // Format: public_key (32) + timestamp (4) + signature (64) + appdata (variable)
  
  MeshCore::DecodedPacket packet;
  memset(&packet, 0, sizeof(packet));
  
  // Set header - FLOOD routing + ADVERT payload type
  packet.routeType = RouteType::FLOOD;
  packet.payloadType = PayloadType::ADVERT;
  packet.payloadVersion = 0;
  packet.hasTransportCodes = false;
  packet.pathLength = 0;
  
  // Build header byte
  packet.header = (static_cast<uint8_t>(packet.routeType) & MeshCore::PH_ROUTE_MASK) |
                  ((static_cast<uint8_t>(packet.payloadType) & MeshCore::PH_TYPE_MASK) << MeshCore::PH_TYPE_SHIFT) |
                  ((packet.payloadVersion & MeshCore::PH_VER_MASK) << MeshCore::PH_VER_SHIFT);
  
  // Get identity
  const uint8_t *publicKey = CryptoIdentity::getInstance().getPublicKey();
  const uint8_t *privateKey = CryptoIdentity::getInstance().getPrivateKey();
  
  // Get timestamp
  uint32_t timestamp = TimeSync::now();
  
  // Build appdata
  uint8_t appData[MeshCore::MAX_ADVERT_DATA_SIZE];
  uint8_t appDataLen = buildAdvertAppData(appData);
  
  // Build payload: public_key + timestamp + signature + appdata
  uint16_t payloadIdx = 0;
  
  // Copy public key (32 bytes)
  memcpy(&packet.payload[payloadIdx], publicKey, MeshCoreCompat::PUB_KEY_SIZE);
  payloadIdx += MeshCoreCompat::PUB_KEY_SIZE;
  
  // Copy timestamp (4 bytes)
  memcpy(&packet.payload[payloadIdx], &timestamp, 4);
  payloadIdx += 4;
  
  // Reserve space for signature (64 bytes) - will fill in later
  uint8_t *signaturePtr = &packet.payload[payloadIdx];
  payloadIdx += 64;
  
  // Copy appdata
  memcpy(&packet.payload[payloadIdx], appData, appDataLen);
  payloadIdx += appDataLen;
  
  packet.payloadLength = payloadIdx;
  
  // Build message to sign: public_key + timestamp + appdata
  uint8_t message[MeshCoreCompat::PUB_KEY_SIZE + 4 + MeshCore::MAX_ADVERT_DATA_SIZE];
  uint16_t messageLen = 0;
  
  memcpy(&message[messageLen], publicKey, MeshCoreCompat::PUB_KEY_SIZE);
  messageLen += MeshCoreCompat::PUB_KEY_SIZE;
  
  memcpy(&message[messageLen], &timestamp, 4);
  messageLen += 4;
  
  memcpy(&message[messageLen], appData, appDataLen);
  messageLen += appDataLen;
  
  // Sign the message
  ed25519_sign(signaturePtr, message, messageLen, publicKey, privateKey);
  
  // Encode packet to wire format
  length = MeshCore::PacketDecoder::encode(packet, dest, 256);
  
  if (length == 0) {
    LOG_ERROR("Failed to encode advert packet");
    return false;
  }
  
  LOG_INFO_FMT("Built advert packet: %d bytes, timestamp: %lu", length, timestamp);
  return true;
}


