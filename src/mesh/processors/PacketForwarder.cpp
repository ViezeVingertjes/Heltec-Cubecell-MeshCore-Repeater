#include "PacketForwarder.h"
#include "../../core/NodeConfig.h"
#include "../../core/PacketDecoder.h"
#include "../../core/PacketValidator.h"
#include <Arduino.h>
#include <string.h>

namespace MeshCore {

ProcessResult PacketForwarder::processPacket(const PacketEvent &event,
                                             ProcessingContext &ctx) {
  if (!Config::Forwarding::ENABLED) {
    return ProcessResult::CONTINUE;
  }

  // Check if packet should be forwarded
  auto forwardCheck = shouldForward(event.packet, event.rssi, ctx);
  if (forwardCheck.isError()) {
    if (forwardCheck.error == ErrorCode::WEAK_SIGNAL ||
        forwardCheck.error == ErrorCode::INVALID_PACKET) {
      // These are normal, just don't forward
      return ProcessResult::CONTINUE;
    }
    LOG_WARN_FMT("Forward check failed: %s",
                 errorCodeToString(forwardCheck.error));
    return ProcessResult::CONTINUE;
  }

  ctx.shouldForward = true;

  // Create copy and add our node to path
  DecodedPacket forwardPacket;
  memcpy(&forwardPacket, &event.packet, sizeof(DecodedPacket));
  
  auto pathResult = addNodeToPath(forwardPacket);
  if (pathResult.isError()) {
    LOG_WARN_FMT("Cannot add to path: %s",
                 errorCodeToString(pathResult.error));
    droppedCount++;
    return ProcessResult::CONTINUE;
  }

  // Encode packet for transmission
  uint8_t rawPacket[Config::Forwarding::MAX_ENCODED_PACKET_SIZE];
  auto encodeResult = encodePacketForForwarding(forwardPacket, rawPacket, sizeof(rawPacket));
  if (encodeResult.isError()) {
    LOG_ERROR("Failed to encode packet for forwarding");
    droppedCount++;
    return ProcessResult::CONTINUE;
  }
  uint16_t length = encodeResult.value;

  // Calculate delays based on signal quality
  uint32_t airtime = LoRaTransmitter::estimateAirtime(length);
  float score = calculatePacketScore(event.snr);
  uint32_t rxDelay = calculateRxDelay(score, airtime);
  uint32_t txJitter = calculateTxJitter(airtime);
  uint32_t totalDelay = rxDelay + txJitter;

  // Forward immediately or queue based on delay
  if (totalDelay < Config::Forwarding::MIN_DELAY_THRESHOLD_MS) {
    handleImmediateForward(rawPacket, length, event.hash);
  } else {
    handleDelayedForward(rawPacket, length, totalDelay, event.snr, airtime);
  }

  return ProcessResult::CONTINUE;
}

void PacketForwarder::loop() {
  if (processDelayQueue()) {
    forwardedCount++;
  }
}

Result<void> PacketForwarder::shouldForward(const DecodedPacket &packet,
                                            int16_t rssi,
                                            const ProcessingContext &ctx) {
  // Don't forward duplicates
  if (ctx.isDuplicate) {
    return Err(ErrorCode::DUPLICATE);
  }

  // Only forward flood packets
  if (packet.routeType != RouteType::FLOOD &&
      packet.routeType != RouteType::TRANSPORT_FLOOD) {
    LOG_DEBUG("Not forwarding non-flood packet");
    return Err(ErrorCode::INVALID_PACKET);
  }

  // Check path length
  if (packet.pathLength >= Config::Forwarding::MAX_PATH_LENGTH) {
    LOG_DEBUG_FMT("Path too long (%d), not forwarding", packet.pathLength);
    return Err(ErrorCode::PATH_TOO_LONG);
  }

  // Check signal strength
  if (rssi < Config::Forwarding::MIN_RSSI_TO_FORWARD) {
    LOG_DEBUG_FMT("Signal too weak (%d dBm), not forwarding", rssi);
    return Err(ErrorCode::WEAK_SIGNAL);
  }

  // Check for routing loops
  uint8_t nodeHash = NodeConfig::getInstance().getNodeHash();
  if (PacketValidator::isNodeInPath(packet, nodeHash)) {
    LOG_DEBUG("Node already in path, not forwarding (loop prevention)");
    return Err(ErrorCode::INVALID_PACKET);
  }

  // Validate packet structure
  auto validationResult = PacketValidator::validate(packet);
  if (validationResult.isError()) {
    LOG_WARN_FMT("Packet validation failed: %s",
                 errorCodeToString(validationResult.error));
    return validationResult;
  }

  return Ok();
}

Result<void> PacketForwarder::transmitPacket(const uint8_t *rawPacket,
                                             uint16_t length) {
  if (rawPacket == nullptr || length == 0) {
    return Err(ErrorCode::INVALID_PARAMETER);
  }

  LoRaTransmitter &transmitter = LoRaTransmitter::getInstance();

  if (transmitter.isTransmitting()) {
    LOG_DEBUG("Transmitter busy, skipping forward");
    return Err(ErrorCode::HARDWARE_ERROR);
  }

  bool success = transmitter.transmit(rawPacket, length);
  return success ? Ok() : Err(ErrorCode::TRANSMIT_FAILED);
}

Result<void> PacketForwarder::addNodeToPath(DecodedPacket &packet) {
  // Validate we can add to path
  auto canAddResult = PacketValidator::canAddToPath(packet);
  if (canAddResult.isError()) {
    return canAddResult;
  }

  uint8_t nodeHash = NodeConfig::getInstance().getNodeHash();

  LOG_INFO_FMT("Adding node hash 0x%02X to path (current path_len=%d)",
               nodeHash, packet.pathLength);

  packet.path[packet.pathLength] = nodeHash;
  packet.pathLength++;

  LOG_INFO_FMT("Path updated, new path_len=%d", packet.pathLength);

  return Ok();
}

Result<uint16_t> PacketForwarder::encodePacketForForwarding(
    const DecodedPacket &packet, uint8_t *buffer, uint16_t bufferSize) {
  uint16_t length = PacketDecoder::encode(packet, buffer, bufferSize);
  if (length == 0) {
    return Err<uint16_t>(ErrorCode::ENCODE_ERROR);
  }
  return Ok<uint16_t>(length);
}

void PacketForwarder::handleImmediateForward(const uint8_t *rawPacket, 
                                             uint16_t length, uint32_t hash) {
  auto txResult = transmitPacket(rawPacket, length);
  if (txResult.isOk()) {
    forwardedCount++;
    LOG_INFO_FMT("Forwarded immediately hash=0x%08lX (total: %lu)",
                 hash, forwardedCount);
  } else {
    LOG_WARN_FMT("Immediate transmit failed: %s",
                 errorCodeToString(txResult.error));
    droppedCount++;
  }
}

void PacketForwarder::handleDelayedForward(const uint8_t *rawPacket, 
                                           uint16_t length, uint32_t totalDelay,
                                           int8_t snr, uint32_t airtime) {
  auto enqueueResult = enqueueDelayed(rawPacket, length, totalDelay);
  if (enqueueResult.isOk()) {
    float score = calculatePacketScore(snr);
    uint32_t scorePercent = static_cast<uint32_t>(score * 100.0f);
    uint32_t rxDelay = calculateRxDelay(score, airtime);
    uint32_t txJitter = calculateTxJitter(airtime);
    LOG_INFO_FMT("Queued for delayed forward: rxDelay=%lu ms, txJitter=%lu "
                 "ms, total=%lu ms (score=%lu%%)",
                 rxDelay, txJitter, totalDelay, scorePercent);
  } else {
    LOG_WARN_FMT("Queue failed: %s", errorCodeToString(enqueueResult.error));
    droppedCount++;
  }
}

float PacketForwarder::calculatePacketScore(int8_t snr) const {
  // Convert SNR from 0.25 dB units to dB
  float snrFloat = static_cast<float>(snr) / Config::Forwarding::SNR_SCALE_FACTOR;
  
  // Normalize to 0.0 - 1.0 range
  float normalized = (snrFloat - Config::Forwarding::SNR_MIN_DB) / 
                     Config::Forwarding::SNR_RANGE_DB;
  
  // Clamp to valid range
  normalized = max(0.0f, min(1.0f, normalized));
  
  return normalized;
}

uint32_t PacketForwarder::calculateRxDelay(float score,
                                           uint32_t airtime) const {
  if (Config::Forwarding::RX_DELAY_BASE <= 0.0f) {
    return 0;
  }

  // Use lookup table to approximate pow(2.5, 0.85 - score) - 1.0
  // Precomputed for RX_DELAY_BASE = 2.5, exponents from -0.15 to 0.85
  // Table has 11 entries for score values 0.0, 0.1, 0.2, ... 1.0
  static const uint16_t multiplierTable[] = {
    1293,  // score=0.0, exp=0.85  -> 2.5^0.85 - 1 = 2.293
    1105,  // score=0.1, exp=0.75  -> 2.5^0.75 - 1 = 2.105
    936,   // score=0.2, exp=0.65  -> 2.5^0.65 - 1 = 1.936
    783,   // score=0.3, exp=0.55  -> 2.5^0.55 - 1 = 1.783
    645,   // score=0.4, exp=0.45  -> 2.5^0.45 - 1 = 1.645
    521,   // score=0.5, exp=0.35  -> 2.5^0.35 - 1 = 1.521
    410,   // score=0.6, exp=0.25  -> 2.5^0.25 - 1 = 1.410
    310,   // score=0.7, exp=0.15  -> 2.5^0.15 - 1 = 1.310
    220,   // score=0.8, exp=0.05  -> 2.5^0.05 - 1 = 1.220
    139,   // score=0.9, exp=-0.05 -> 2.5^-0.05 - 1 = 1.139
    65     // score=1.0, exp=-0.15 -> 2.5^-0.15 - 1 = 1.065
  };

  // Convert score to table index (0-10)
  uint8_t index = static_cast<uint8_t>(score * 10.0f);
  if (index > 10) index = 10;

  // Get multiplier from table (scaled by 1000)
  uint32_t multiplier = multiplierTable[index];

  // Calculate delay: (multiplier / 1000) * airtime
  uint32_t rxDelay = (multiplier * airtime) / 1000;
  return rxDelay;
}

uint32_t PacketForwarder::calculateTxJitter(uint32_t airtime) const {
  uint32_t slotTime =
      static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
  uint32_t randomSlot = random(0, Config::Forwarding::TX_DELAY_JITTER_SLOTS);
  return randomSlot * slotTime;
}

Result<void> PacketForwarder::enqueueDelayed(const uint8_t *encodedPacket,
                                             uint16_t length,
                                             uint32_t delayMs) {
  // Validate parameters
  if (encodedPacket == nullptr) {
    return Err(ErrorCode::INVALID_PARAMETER);
  }

  if (length > Config::Forwarding::MAX_ENCODED_PACKET_SIZE) {
    LOG_ERROR("Packet too large for delay queue");
    return Err(ErrorCode::BUFFER_TOO_SMALL);
  }

  if (delayQueue.isFull()) {
    LOG_WARN("Delayed forward queue full, dropping packet");
    return Err(ErrorCode::QUEUE_FULL);
  }

  uint32_t scheduledTime = millis() + delayMs;

  DelayedPacket delayed;
  memcpy(delayed.encodedPacket, encodedPacket, length);
  delayed.packetLength = length;
  delayed.scheduledTime = scheduledTime;
  delayed.valid = true;

  bool inserted = delayQueue.insert(delayed, scheduledTime);
  if (!inserted) {
    return Err(ErrorCode::QUEUE_FULL);
  }

  delayedCount++;

  LOG_DEBUG_FMT(
      "Queued packet for delayed forward in %lu ms (delayed count: %lu)",
      delayMs, delayedCount);

  return Ok();
}

bool PacketForwarder::processDelayQueue() {
  uint32_t now = millis();
  bool processed = false;

  // Process ALL ready packets in one iteration for lower latency
  while (true) {
    uint32_t frontTime;
    if (!delayQueue.peekFrontKey(frontTime)) {
      break; // Queue empty
    }

    if (now < frontTime) {
      break; // Next packet not ready yet
    }

    // Check if transmitter is available BEFORE popping from queue
    // This matches PingResponder behavior - don't lose packet if busy
    LoRaTransmitter &transmitter = LoRaTransmitter::getInstance();
    if (transmitter.isTransmitting()) {
      // Transmitter busy, leave packet in queue and retry next loop
      break;
    }

    // Now pop the packet - we know transmitter is available
    DelayedPacket delayed;
    if (delayQueue.popFront(delayed)) {
      auto txResult =
          transmitPacket(delayed.encodedPacket, delayed.packetLength);
      if (txResult.isOk()) {
        LOG_DEBUG("Transmitted delayed packet");
        processed = true;
        // Successfully transmitted, continue to next packet
      } else {
        // Transmit failed - re-insert with retry delay
        // This matches PingResponder behavior - retry until success
        uint32_t airtime = LoRaTransmitter::estimateAirtime(delayed.packetLength);
        uint32_t retryDelay = static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
        uint32_t retryTime = millis() + retryDelay;
        
        if (!delayQueue.insert(delayed, retryTime)) {
          // Queue full, packet dropped
          LOG_WARN_FMT("Failed to re-insert packet: %s",
                       errorCodeToString(txResult.error));
          droppedCount++;
        }
        // Stop processing queue after retry to avoid busy-looping
        break;
      }
    } else {
      break; // Failed to pop, shouldn't happen
    }
  }

  return processed;
}

} // namespace MeshCore
