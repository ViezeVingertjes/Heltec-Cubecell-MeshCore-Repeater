#include "routing/repeater.h"
#include "core/config.h"
#include <cstring>
namespace MiniCore {
using Config::REPEATER_SLOT_COUNT;
using Config::ADVERT_MIN_SIZE;
using Config::PRIORITY_DIRECT;
using Config::PRIORITY_FLOOD_DEFAULT;
using Config::PRIORITY_PATH;
using Config::PRIORITY_ADVERT;
using Config::MAX_HASH_SIZE;
Repeater::Repeater(PacketHistory& history, const RepeaterConfig& config)
    : history_(history)
    , config_(config)
    , selfHash_{}
{
}
void Repeater::setSelfHash(uint8_t hash) {
    std::memset(selfHash_, 0, MAX_HASH_SIZE);
    selfHash_[0] = hash;
}
void Repeater::setSelfHashMultiByte(const uint8_t* hash, uint8_t len) {
    std::memset(selfHash_, 0, MAX_HASH_SIZE);
    if (hash == nullptr || len == 0) {
        return;
    }
    uint8_t copyLen = (len > MAX_HASH_SIZE) ? MAX_HASH_SIZE : len;
    std::memcpy(selfHash_, hash, copyLen);
}
bool Repeater::isHashMatch(const uint8_t* hash, uint8_t len) const {
    if (hash == nullptr) {
        return false;
    }
    if (len > MAX_HASH_SIZE) {
        len = MAX_HASH_SIZE;
    }
    return std::memcmp(selfHash_, hash, len) == 0;
}
bool Repeater::isFloodPacketDestinedForSelf(const Packet& packet) const {
    if (selfHash_[0] == 0) {
        return false;
    }
    auto type = packet.getPayloadType();
    if (type == PayloadType::Request ||
        type == PayloadType::Response ||
        type == PayloadType::TextMessage ||
        type == PayloadType::AnonRequest ||
        type == PayloadType::Path) {
        if (packet.payloadLen > 0 && packet.payload[0] == selfHash_[0]) {
            return true;
        }
    }
    return false;
}
namespace {
constexpr uint8_t TRACE_HEADER_SIZE = 9;
constexpr uint8_t ADVERT_NODE_TYPE_MASK = 0x0F;
}
bool Repeater::isRepeaterOrRoomAdvert(const Packet& packet) const {
    using Config::ADV_TYPE_REPEATER;
    using Config::ADV_TYPE_ROOM;
    if (packet.payloadLen <= ADVERT_MIN_SIZE) {
        return false;
    }
    uint8_t nodeType = packet.payload[ADVERT_MIN_SIZE] & ADVERT_NODE_TYPE_MASK;
    return (nodeType == ADV_TYPE_REPEATER || nodeType == ADV_TYPE_ROOM);
}
bool Repeater::hasSeenPacket(const Packet& packet) {
    return history_.hasSeen(packet);
}
RepeaterAction Repeater::shouldForward(const Packet& packet) {
    if (!config_.enabled) {
        return RepeaterAction::Drop;
    }
    if (packet.getPayloadType() == PayloadType::Control) {
        return RepeaterAction::Drop;
    }
    if (packet.getPayloadType() == PayloadType::RawCustom && packet.isFloodRoute()) {
        return RepeaterAction::Drop;
    }
    if (config_.dropRepeaterAdverts && packet.getPayloadType() == PayloadType::Advert) {
        if (isRepeaterOrRoomAdvert(packet)) {
            return RepeaterAction::Drop;
        }
    }
    if (packet.isFloodRoute()) {
        if (packet.pathLen >= config_.maxFloodPath) {
            return RepeaterAction::Drop;
        }
        if (isFloodPacketDestinedForSelf(packet)) {
            return RepeaterAction::Drop;
        }
        if (hasSeenPacket(packet)) {
            return RepeaterAction::Drop;
        }
        return RepeaterAction::Forward;
    }
    if (packet.isDirectRoute()) {
        if (packet.getPayloadType() == PayloadType::Trace) {
            return shouldForwardTrace(packet);
        }
        if (packet.pathLen == 0) {
            return RepeaterAction::Drop;
        }
        if (packet.path[0] != selfHash_[0]) {
            return RepeaterAction::Drop;
        }
        if (packet.pathLen == 1) {
            return RepeaterAction::Drop;
        }
        if (hasSeenPacket(packet)) {
            return RepeaterAction::Drop;
        }
        return RepeaterAction::Forward;
    }
    return RepeaterAction::Drop;
}
uint32_t Repeater::calculateRetransmitDelay(const Packet& packet, uint32_t baseAirtime) const {
    if (packet.getPayloadType() == PayloadType::Trace) {
        return 0;
    }
    uint32_t slot = static_cast<uint32_t>(packet.pathLen % REPEATER_SLOT_COUNT);
    return slot * baseAirtime;
}
uint8_t Repeater::getPriority(const Packet& packet) const {
    if (packet.isDirectRoute()) {
        return PRIORITY_DIRECT;
    }
    auto payloadType = packet.getPayloadType();
    if (payloadType == PayloadType::Advert) {
        return PRIORITY_ADVERT;
    }
    if (payloadType == PayloadType::Path) {
        return PRIORITY_PATH;
    }
    return PRIORITY_FLOOD_DEFAULT;
}
void Repeater::removeSelfFromPath(Packet& packet) {
    if (packet.pathLen == 0) {
        return;
    }
    for (uint8_t i = 0; i < packet.pathLen - 1; ++i) {
        packet.path[i] = packet.path[i + 1];
    }
    --packet.pathLen;
}
void Repeater::appendSnrToPath(Packet& packet) {
    if (packet.pathLen >= MAX_PATH_SIZE) {
        return;
    }
    int8_t snrScaled = static_cast<int8_t>(packet.snr * 4);
    packet.path[packet.pathLen++] = static_cast<uint8_t>(snrScaled);
}
RepeaterAction Repeater::shouldForwardTrace(const Packet& packet) {
    if (packet.pathLen >= MAX_PATH_SIZE) {
        return RepeaterAction::Drop;
    }
    if (packet.payloadLen < TRACE_HEADER_SIZE) {
        return RepeaterAction::Drop;
    }
    uint8_t flags = packet.payload[8];
    uint8_t pathHashSize = 1 << (flags & 0x03);
    uint8_t hashesStart = TRACE_HEADER_SIZE;
    uint8_t hashesLen = packet.payloadLen - TRACE_HEADER_SIZE;
    uint8_t offset = packet.pathLen * pathHashSize;
    if (offset >= hashesLen) {
        return RepeaterAction::Drop;
    }
    if (!isHashMatch(&packet.payload[hashesStart + offset], pathHashSize)) {
        return RepeaterAction::Drop;
    }
    if (hasSeenPacket(packet)) {
        return RepeaterAction::Drop;
    }
    return RepeaterAction::Forward;
}
Status Repeater::appendSelfHash(Packet& packet, uint8_t selfHash) {
    if (packet.pathLen >= MAX_PATH_SIZE) {
        return ErrorCode::BufferOverflow;
    }
    packet.path[packet.pathLen] = selfHash;
    ++packet.pathLen;
    return Status();
}
}
