#include "CommandHandler.h"

#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../core/HashUtils.h"
#include "../../core/Logger.h"
#include "../../core/NodeConfig.h"
#include "../../core/TimeSync.h"
#include "../../core/Config.h"
#include "../../core/CryptoIdentity.h"
#include "../../power/PowerManager.h"
#include "../../radio/LoRaReceiver.h"
#include "../../radio/LoRaTransmitter.h"
#include "../channels/PrivateChannelAnnouncer.h"
#include "../channels/ChannelAnnouncer.h"
#include "../NeighborTracker.h"
#include "../../lib/ed25519/ed_25519.h"

using MeshCore::PayloadType;
using MeshCore::RouteType;

uint32_t CommandHandler::hashPayload(const MeshCore::DecodedPacket &packet) {
  return HashUtils::fnv1a(packet.payload, packet.payloadLength);
}

MeshCore::ProcessResult
CommandHandler::processPacket(const MeshCore::PacketEvent &event,
                               MeshCore::ProcessingContext &) {
  if (event.packet.payloadType != PayloadType::GRP_TXT) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  uint32_t msgTimestamp = 0;
  char text[ChannelAnnouncer::MAX_MESSAGE_LEN];
  uint8_t privateChannelIndex = 0;
  
  // Only allow commands in private channels
  if (!PrivateChannelAnnouncer::getInstance().decodeMessage(event.packet,
                                                            msgTimestamp, text,
                                                            sizeof(text),
                                                            privateChannelIndex)) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Extract command (skip username prefix if present)
  const char *content = text;
  const char *colon = strchr(text, ':');
  if (colon != nullptr) {
    content = colon + 1;
    while (*content == ' ') {
      ++content;
    }
  }

  // Check if it's a command we handle
  if (*content != '!') {
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Parse command and arguments
  char cmd[32];
  const char *args = nullptr;
  const char *space = strchr(content, ' ');
  if (space) {
    size_t cmdLen = space - content;
    if (cmdLen >= sizeof(cmd)) cmdLen = sizeof(cmd) - 1;
    memcpy(cmd, content, cmdLen);
    cmd[cmdLen] = '\0';
    args = space + 1;
    while (*args == ' ') ++args;  // Skip leading spaces
  } else {
    strncpy(cmd, content, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
  }

  // Check for target specifier (@XX or @all)
  // Format: !command @target args
  // Examples: !status @5A, !advert @all, !location @42 lat lon
  uint8_t myHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  bool targetedToMe = true;  // Default: respond to untargeted commands
  
  if (args && *args == '@') {
    // Command has a target specifier
    const char *target = args + 1;  // Skip '@'
    const char *nextSpace = strchr(target, ' ');
    
    // Extract target string
    char targetStr[8];
    if (nextSpace) {
      size_t targetLen = nextSpace - target;
      if (targetLen >= sizeof(targetStr)) targetLen = sizeof(targetStr) - 1;
      memcpy(targetStr, target, targetLen);
      targetStr[targetLen] = '\0';
      args = nextSpace + 1;  // Args start after target
      while (*args == ' ') ++args;
    } else {
      strncpy(targetStr, target, sizeof(targetStr) - 1);
      targetStr[sizeof(targetStr) - 1] = '\0';
      args = nullptr;  // No args after target
    }
    
    // Check if target matches this node
    if (strcmp(targetStr, "all") == 0) {
      targetedToMe = true;  // Explicit broadcast
    } else {
      // Parse hex hash (e.g., "5A" or "5a")
      uint32_t targetHash;
      char *endptr;
      targetHash = strtoul(targetStr, &endptr, 16);
      
      if (endptr != targetStr && *endptr == '\0') {
        // Valid hex parsed
        targetedToMe = (targetHash == myHash);
      } else {
        // Invalid target format, ignore command
        return MeshCore::ProcessResult::CONTINUE;
      }
    }
  }
  
  // If command is not targeted to this node, ignore it
  if (!targetedToMe) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Rate limiting: only respond once per minute
  uint32_t now = millis();
  if (lastResponseTime != 0 && (now - lastResponseTime) < RESPONSE_RATE_LIMIT_MS) {
    return MeshCore::ProcessResult::CONTINUE;
  }

  // Deduplicate based on payload hash
  uint32_t payloadHash = hashPayload(event.packet);
  if (payloadHash == lastPayloadHash && (now - lastPayloadTime) < DEDUP_TIMEOUT_MS) {
    return MeshCore::ProcessResult::CONTINUE;
  }
  lastPayloadHash = payloadHash;
  lastPayloadTime = now;

  // Dispatch to command handlers
  bool handled = false;
  if (strcmp(cmd, "!status") == 0) {
    handled = handleStatusCommand(args, privateChannelIndex);
  } else if (strcmp(cmd, "!advert") == 0) {
    handled = handleAdvertCommand(privateChannelIndex);
  } else if (strcmp(cmd, "!location") == 0) {
    handled = handleLocationCommand(args, privateChannelIndex);
  } else if (strcmp(cmd, "!neighbors") == 0 || strcmp(cmd, "!neighbours") == 0) {
    handled = handleNeighborsCommand(privateChannelIndex);
  } else if (strcmp(cmd, "!help") == 0) {
    handled = handleHelpCommand(privateChannelIndex);
  }
  
  if (handled) {
    LOG_INFO_FMT("Processed targeted command: %s", cmd);
  }

  return handled ? MeshCore::ProcessResult::CONTINUE : MeshCore::ProcessResult::CONTINUE;
}

bool CommandHandler::handleStatusCommand(const char *args, uint8_t privateChannelIndex) {
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  char message[ChannelAnnouncer::MAX_MESSAGE_LEN];
  
  // Check if this is "!status clear"
  if (args && strcmp(args, "clear") == 0) {
    // Clear statistics
    PowerManager::getInstance().resetStats();
    LoRaReceiver::resetStats();
    LoRaTransmitter::getInstance().resetStats();
    
    snprintf(message, sizeof(message), "%s %02X: Stats cleared", 
             Config::Identity::NODE_NAME, nodeHash);
  } else {
    // Show status
    uint32_t rxPackets = LoRaReceiver::getPacketCount();
    uint32_t txPackets = LoRaTransmitter::getInstance().getTransmitCount();
    uint32_t rxAirtime = LoRaReceiver::getTotalRxAirtimeMs();
    uint32_t txAirtime = LoRaTransmitter::getInstance().getTotalAirtimeMs();
    uint32_t totalAirtime = rxAirtime + txAirtime;
    uint32_t airtimeSec = totalAirtime / 1000;
    
    snprintf(message, sizeof(message), "%s %02X: RX:%lu TX:%lu Air:%lus", 
             Config::Identity::NODE_NAME, nodeHash, rxPackets, txPackets, 
             airtimeSec);
  }

  uint32_t timestamp = TimeSync::now();
  bool buildSuccess = PrivateChannelAnnouncer::getInstance().buildPacket(message, pendingPacket, 
                                                                          pendingPacketLength, timestamp,
                                                                          privateChannelIndex);
  
  if (!buildSuccess) {
    LOG_WARN("Failed to build status response");
    return false;
  }
  
  uint32_t jitter = calculateResponseDelay(pendingPacketLength);
  pendingResponse = true;
  responseTime = millis() + jitter;
  
  LOG_INFO("Queued !status response");
  return true;
}

bool CommandHandler::handleAdvertCommand(uint8_t privateChannelIndex) {
  // Build advert packet
  if (!buildAdvertPacket(pendingPacket, pendingPacketLength)) {
    LOG_WARN("Failed to build advert packet");
    return false;
  }

  uint32_t jitter = calculateResponseDelay(pendingPacketLength);
  pendingResponse = true;
  responseTime = millis() + jitter;
  
  LOG_INFO_FMT("Queued !advert response (%u bytes)", pendingPacketLength);
  return true;
}

bool CommandHandler::handleLocationCommand(const char *args, uint8_t privateChannelIndex) {
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  char message[ChannelAnnouncer::MAX_MESSAGE_LEN];
  
  if (!args || *args == '\0') {
    // Query current location
    if (MeshCore::NodeConfig::getInstance().hasLocation()) {
      int32_t lat = MeshCore::NodeConfig::getInstance().getLatitude();
      int32_t lon = MeshCore::NodeConfig::getInstance().getLongitude();
      // Display as microdegrees (raw format, more compact)
      snprintf(message, sizeof(message), "%s %02X: Loc %ld,%ld", 
               Config::Identity::NODE_NAME, nodeHash, lat, lon);
    } else {
      snprintf(message, sizeof(message), "%s %02X: No loc", 
               Config::Identity::NODE_NAME, nodeHash);
    }
  } else if (strcmp(args, "clear") == 0) {
    // Clear location
    MeshCore::NodeConfig::getInstance().clearLocation();
    snprintf(message, sizeof(message), "%s %02X: Loc cleared", 
             Config::Identity::NODE_NAME, nodeHash);
  } else {
    // Parse "lat lon" as integers (microdegrees)
    char *endptr;
    int32_t lat = strtol(args, &endptr, 10);
    if (endptr == args) {
      snprintf(message, sizeof(message), "%s %02X: Bad lat", 
               Config::Identity::NODE_NAME, nodeHash);
    } else {
      int32_t lon = strtol(endptr, &endptr, 10);
      if (endptr == args) {
        snprintf(message, sizeof(message), "%s %02X: Bad lon", 
                 Config::Identity::NODE_NAME, nodeHash);
      } else {
        MeshCore::NodeConfig::getInstance().setLocation(lat, lon);
        snprintf(message, sizeof(message), "%s %02X: Loc set", 
                 Config::Identity::NODE_NAME, nodeHash);
      }
    }
  }
  
  uint32_t timestamp = TimeSync::now();
  bool buildSuccess = PrivateChannelAnnouncer::getInstance().buildPacket(message, pendingPacket, 
                                                                          pendingPacketLength, timestamp,
                                                                          privateChannelIndex);
  
  if (!buildSuccess) {
    LOG_WARN("Failed to build location response");
    return false;
  }
  
  uint32_t jitter = calculateResponseDelay(pendingPacketLength);
  pendingResponse = true;
  responseTime = millis() + jitter;
  
  LOG_INFO("Queued !location response");
  return true;
}

bool CommandHandler::handleNeighborsCommand(uint8_t privateChannelIndex) {
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  char message[ChannelAnnouncer::MAX_MESSAGE_LEN];
  
  uint8_t count = NeighborTracker::getInstance().getNeighborCount();
  
  // Format: "VieZe Rogue 5A: N:3 5A3F:12 7C42:8 1234:-5"
  int offset = snprintf(message, sizeof(message), "%s %02X: N:%u ", 
                       Config::Identity::NODE_NAME, nodeHash, count);
  
  if (count > 0 && offset > 0 && offset < (int)sizeof(message)) {
    NeighborTracker::getInstance().buildNeighborList(&message[offset], 
                                                     sizeof(message) - offset);
  }
  
  uint32_t timestamp = TimeSync::now();
  bool buildSuccess = PrivateChannelAnnouncer::getInstance().buildPacket(message, pendingPacket, 
                                                                          pendingPacketLength, timestamp,
                                                                          privateChannelIndex);
  
  if (!buildSuccess) {
    LOG_WARN("Failed to build neighbors response");
    return false;
  }
  
  uint32_t jitter = calculateResponseDelay(pendingPacketLength);
  pendingResponse = true;
  responseTime = millis() + jitter;
  
  LOG_INFO("Queued !neighbors response");
  return true;
}

bool CommandHandler::handleHelpCommand(uint8_t privateChannelIndex) {
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  char message[ChannelAnnouncer::MAX_MESSAGE_LEN];
  
  // Clear hierarchical format (under 160 chars)
  snprintf(message, sizeof(message), 
           "%s %02X: !cmd[@XX] | !status[clear] !location[lat lon|clear] !neighbors !advert !help", 
           Config::Identity::NODE_NAME, nodeHash);
  
  uint32_t timestamp = TimeSync::now();
  bool buildSuccess = PrivateChannelAnnouncer::getInstance().buildPacket(message, pendingPacket, 
                                                                          pendingPacketLength, timestamp,
                                                                          privateChannelIndex);
  
  if (!buildSuccess) {
    LOG_WARN("Failed to build help response");
    return false;
  }
  
  uint32_t jitter = calculateResponseDelay(pendingPacketLength);
  pendingResponse = true;
  responseTime = millis() + jitter;
  
  LOG_INFO("Queued !help response");
  return true;
}

void CommandHandler::loop() {
  if (!pendingResponse) {
    return;
  }
  
  if (millis() >= responseTime) {
    auto &tx = LoRaTransmitter::getInstance();
    if (tx.isTransmitting()) {
      uint32_t airtime = LoRaTransmitter::estimateAirtime(pendingPacketLength);
      uint32_t retryDelay = static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
      responseTime = millis() + retryDelay;
      return;
    }
    
    if (!tx.transmit(pendingPacket, pendingPacketLength)) {
      uint32_t airtime = LoRaTransmitter::estimateAirtime(pendingPacketLength);
      uint32_t retryDelay = static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
      responseTime = millis() + retryDelay;
      return;
    }
    
    lastResponseTime = millis();
    pendingResponse = false;
  }
}

uint32_t CommandHandler::calculateResponseDelay(uint16_t packetLength) const {
  uint32_t airtime = LoRaTransmitter::estimateAirtime(packetLength);
  uint32_t slotTime = static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
  uint32_t randomSlot = random(0, 10);
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  uint32_t hashSlot = nodeHash % 10;
  return (randomSlot + hashSlot) * slotTime;
}

uint8_t CommandHandler::buildAdvertAppData(uint8_t *appData) {
  uint8_t flags = static_cast<uint8_t>(MeshCore::AdvertType::REPEATER);
  
  // Check NodeConfig for location (which may be from EEPROM or config)
  if (MeshCore::NodeConfig::getInstance().hasLocation()) {
    flags |= MeshCore::ADV_LATLON_MASK;
  }
  
  flags |= MeshCore::ADV_NAME_MASK;
  
  appData[0] = flags;
  int idx = 1;
  
  // Add location if available
  if (MeshCore::NodeConfig::getInstance().hasLocation()) {
    int32_t lat = MeshCore::NodeConfig::getInstance().getLatitude();
    int32_t lon = MeshCore::NodeConfig::getInstance().getLongitude();
    memcpy(&appData[idx], &lat, 4);
    idx += 4;
    memcpy(&appData[idx], &lon, 4);
    idx += 4;
  }
  
  // Format name with node hash
  uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
  char nameBuffer[MeshCore::MAX_ADVERT_DATA_SIZE];
  snprintf(nameBuffer, sizeof(nameBuffer), "%s %02X", 
           Config::Identity::NODE_NAME, nodeHash);
  
  const char *name = nameBuffer;
  while (*name && idx < MeshCore::MAX_ADVERT_DATA_SIZE) {
    appData[idx++] = *name++;
  }
  
  return idx;
}

bool CommandHandler::buildAdvertPacket(uint8_t *dest, uint16_t &length) {
  MeshCore::DecodedPacket packet;
  memset(&packet, 0, sizeof(packet));
  
  packet.routeType = RouteType::FLOOD;
  packet.payloadType = PayloadType::ADVERT;
  packet.payloadVersion = 0;
  packet.hasTransportCodes = false;
  packet.pathLength = 0;
  
  packet.header = (static_cast<uint8_t>(packet.routeType) & MeshCore::PH_ROUTE_MASK) |
                  ((static_cast<uint8_t>(packet.payloadType) & MeshCore::PH_TYPE_MASK) << MeshCore::PH_TYPE_SHIFT) |
                  ((packet.payloadVersion & MeshCore::PH_VER_MASK) << MeshCore::PH_VER_SHIFT);
  
  const uint8_t *publicKey = CryptoIdentity::getInstance().getPublicKey();
  const uint8_t *privateKey = CryptoIdentity::getInstance().getPrivateKey();
  uint32_t timestamp = TimeSync::now();
  
  uint8_t appData[MeshCore::MAX_ADVERT_DATA_SIZE];
  uint8_t appDataLen = buildAdvertAppData(appData);
  
  uint16_t payloadIdx = 0;
  memcpy(&packet.payload[payloadIdx], publicKey, MeshCoreCompat::PUB_KEY_SIZE);
  payloadIdx += MeshCoreCompat::PUB_KEY_SIZE;
  
  memcpy(&packet.payload[payloadIdx], &timestamp, 4);
  payloadIdx += 4;
  
  uint8_t *signaturePtr = &packet.payload[payloadIdx];
  payloadIdx += 64;
  
  memcpy(&packet.payload[payloadIdx], appData, appDataLen);
  payloadIdx += appDataLen;
  
  packet.payloadLength = payloadIdx;
  
  // Build message to sign
  uint8_t message[MeshCoreCompat::PUB_KEY_SIZE + 4 + MeshCore::MAX_ADVERT_DATA_SIZE];
  uint16_t messageLen = 0;
  
  memcpy(&message[messageLen], publicKey, MeshCoreCompat::PUB_KEY_SIZE);
  messageLen += MeshCoreCompat::PUB_KEY_SIZE;
  
  memcpy(&message[messageLen], &timestamp, 4);
  messageLen += 4;
  
  memcpy(&message[messageLen], appData, appDataLen);
  messageLen += appDataLen;
  
  ed25519_sign(signaturePtr, message, messageLen, publicKey, privateKey);
  
  length = MeshCore::PacketDecoder::encode(packet, dest, 256);
  
  if (length == 0) {
    LOG_ERROR("Failed to encode advert packet");
    return false;
  }
  
  return true;
}

