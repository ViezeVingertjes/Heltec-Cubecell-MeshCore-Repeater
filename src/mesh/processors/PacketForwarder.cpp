#include "PacketForwarder.h"
#include "../../core/PacketDecoder.h"
#include "../../core/NodeConfig.h"
#include "Deduplicator.h"
#include <string.h>
#include <Arduino.h>
#include <math.h>

namespace MeshCore {

ProcessResult PacketForwarder::processPacket(const PacketEvent& event, ProcessingContext& ctx) {
    if (!Config::Forwarding::ENABLED) {
        return ProcessResult::CONTINUE;
    }
    
    if (!shouldForward(event.packet, event.rssi, ctx)) {
        return ProcessResult::CONTINUE;
    }
    
    ctx.shouldForward = true;
    
    DecodedPacket forwardPacket;
    memcpy(&forwardPacket, &event.packet, sizeof(DecodedPacket));
    
    if (!addNodeToPath(forwardPacket)) {
        LOG_WARN("Cannot add to path, dropping packet");
        droppedCount++;
        return ProcessResult::CONTINUE;
    }
    
    uint8_t rawPacket[256];
    uint16_t length = PacketDecoder::encode(forwardPacket, rawPacket, sizeof(rawPacket));
    if (length == 0) {
        LOG_ERROR("Failed to encode packet for forwarding");
        droppedCount++;
        return ProcessResult::CONTINUE;
    }
    
    uint32_t airtime = LoRaTransmitter::estimateAirtime(length);
    float score = calculatePacketScore(event.snr);
    uint32_t rxDelay = calculateRxDelay(score, airtime);
    uint32_t txJitter = calculateTxJitter(airtime);
    uint32_t totalDelay = rxDelay + txJitter;
    
    if (totalDelay < Config::Forwarding::MIN_DELAY_THRESHOLD_MS) {
        if (transmitPacket(rawPacket, length)) {
            forwardedCount++;
            uint32_t hash = Deduplicator::computePacketHash(event.packet);
            LOG_INFO_FMT("Forwarded immediately hash=0x%08lX (total: %lu)", hash, forwardedCount);
        } else {
            droppedCount++;
        }
    } else {
        if (enqueueDelayed(rawPacket, length, totalDelay)) {
            uint32_t scorePercent = static_cast<uint32_t>(score * 100.0f);
            LOG_INFO_FMT("Queued for delayed forward: rxDelay=%lu ms, txJitter=%lu ms, total=%lu ms (score=%lu%%)", 
                        rxDelay, txJitter, totalDelay, scorePercent);
        } else {
            droppedCount++;
        }
    }
    
    return ProcessResult::CONTINUE;
}

void PacketForwarder::loop() {
    if (processDelayQueue()) {
        forwardedCount++;
    }
}

bool PacketForwarder::shouldForward(const DecodedPacket& packet, int16_t rssi, 
                                   const ProcessingContext& ctx) {
    if (ctx.isDuplicate) {
        return false;
    }
    
    if (packet.routeType != RouteType::FLOOD && 
        packet.routeType != RouteType::TRANSPORT_FLOOD) {
        LOG_DEBUG("Not forwarding non-flood packet");
        return false;
    }
    
    if (packet.pathLength >= Config::Forwarding::MAX_PATH_LENGTH) {
        LOG_DEBUG_FMT("Path too long (%d), not forwarding", packet.pathLength);
        return false;
    }
    
    if (rssi < Config::Forwarding::MIN_RSSI_TO_FORWARD) {
        LOG_DEBUG_FMT("Signal too weak (%d dBm), not forwarding", rssi);
        return false;
    }
    
    return true;
}

bool PacketForwarder::transmitPacket(const uint8_t* rawPacket, uint16_t length) {
    LoRaTransmitter& transmitter = LoRaTransmitter::getInstance();
    
    if (transmitter.isTransmitting()) {
        LOG_DEBUG("Transmitter busy, skipping forward");
        return false;
    }
    
    return transmitter.transmit(rawPacket, length);
}

bool PacketForwarder::addNodeToPath(DecodedPacket& packet) {
    uint8_t nodeHash = NodeConfig::getInstance().getNodeHash();
    
    if (packet.pathLength + 1 > MAX_PATH_SIZE) {
        return false;
    }
    
    LOG_INFO_FMT("Adding node hash 0x%02X to path (current path_len=%d)", nodeHash, packet.pathLength);
    
    packet.path[packet.pathLength] = nodeHash;
    packet.pathLength++;
    
    LOG_INFO_FMT("Path updated, new path_len=%d", packet.pathLength);
    
    return true;
}

float PacketForwarder::calculatePacketScore(int8_t snr) const {
    float snrFloat = static_cast<float>(snr) / 4.0f;
    float normalized = (snrFloat + 20.0f) / 40.0f;
    normalized = max(0.0f, min(1.0f, normalized));
    return normalized;
}

uint32_t PacketForwarder::calculateRxDelay(float score, uint32_t airtime) const {
    if (Config::Forwarding::RX_DELAY_BASE <= 0.0f) {
        return 0;
    }
    
    float exponent = 0.85f - score;
    float multiplier = pow(Config::Forwarding::RX_DELAY_BASE, exponent) - 1.0f;
    
    if (multiplier < 0.0f) {
        multiplier = 0.0f;
    }
    
    uint32_t delay = static_cast<uint32_t>(multiplier * airtime);
    return delay;
}

uint32_t PacketForwarder::calculateTxJitter(uint32_t airtime) const {
    uint32_t slotTime = static_cast<uint32_t>(airtime * Config::Forwarding::TX_DELAY_FACTOR);
    uint32_t randomSlot = random(0, Config::Forwarding::TX_DELAY_JITTER_SLOTS);
    return randomSlot * slotTime;
}

bool PacketForwarder::enqueueDelayed(const uint8_t* encodedPacket, uint16_t length, uint32_t delayMs) {
    if (length > 256) {
        LOG_ERROR("Packet too large for delay queue");
        return false;
    }
    
    size_t freeSlot = DELAY_QUEUE_SIZE;
    for (size_t i = 0; i < DELAY_QUEUE_SIZE; ++i) {
        if (!delayQueue[i].valid) {
            freeSlot = i;
            break;
        }
    }
    
    if (freeSlot >= DELAY_QUEUE_SIZE) {
        LOG_WARN("Delayed forward queue full, dropping packet");
        return false;
    }
    
    uint32_t scheduledTime = millis() + delayMs;
    
    size_t insertIdx = freeSlot;
    for (size_t i = 0; i < DELAY_QUEUE_SIZE; ++i) {
        if (delayQueue[i].valid && delayQueue[i].scheduledTime > scheduledTime) {
            insertIdx = i;
            break;
        }
    }
    
    if (insertIdx != freeSlot) {
        for (size_t i = freeSlot; i > insertIdx; --i) {
            memcpy(&delayQueue[i], &delayQueue[i - 1], sizeof(DelayedPacket));
        }
    }
    
    memcpy(delayQueue[insertIdx].encodedPacket, encodedPacket, length);
    delayQueue[insertIdx].packetLength = length;
    delayQueue[insertIdx].scheduledTime = scheduledTime;
    delayQueue[insertIdx].valid = true;
    delayedCount++;
    
    LOG_DEBUG_FMT("Queued packet for delayed forward in %lu ms (delayed count: %lu)", delayMs, delayedCount);
    
    return true;
}

bool PacketForwarder::processDelayQueue() {
    uint32_t now = millis();
    bool processed = false;
    
    for (size_t i = 0; i < DELAY_QUEUE_SIZE; ++i) {
        if (!delayQueue[i].valid) continue;
        
        if (now >= delayQueue[i].scheduledTime) {
            if (transmitPacket(delayQueue[i].encodedPacket, delayQueue[i].packetLength)) {
                LOG_DEBUG("Transmitted delayed packet");
                processed = true;
            } else {
                return processed;
            }
            
            for (size_t j = i; j < DELAY_QUEUE_SIZE - 1; ++j) {
                memcpy(&delayQueue[j], &delayQueue[j + 1], sizeof(DelayedPacket));
            }
            delayQueue[DELAY_QUEUE_SIZE - 1].valid = false;
            i--;
        } else {
            break;
        }
    }
    
    return processed;
}

}

