#include "routing/packet_router.h"
#include "time/time_sync.h"
namespace MiniCore {
constexpr uint32_t BASE_AIRTIME_MS = Config::REPEATER_SLOT_AIRTIME_MS;
PacketRouter::PacketRouter(Repeater& repeater)
    : repeater_(repeater)
    , history_(repeater.history())
{
}
void PacketRouter::setSelfHash(uint8_t hash) {
    selfHash_ = hash;
    repeater_.setSelfHash(hash);
}
bool PacketRouter::checkAndMarkSeen(const Packet& packet) {
    if (isValidControlRequest(packet)) {
        return history_.hasSeen(packet);
    }
        return history_.contains(packet);
}
void PacketRouter::processPacket(Packet& packet, const RxPacket& rxInfo) {
    packet.snr = rxInfo.snr;
        bool isDuplicate = checkAndMarkSeen(packet);
        auto payloadType = packet.getPayloadType();
        if (payloadType == PayloadType::Advert) {
        processAdvert(packet);
    }
        if (isValidControlRequest(packet)) {
        processControlRequest(packet, rxInfo.snr, isDuplicate);
        return;
    }
        processForwardable(packet, rxInfo, isDuplicate);
}
void PacketRouter::notifyAdvertReceived(uint8_t senderHash, uint32_t timestamp) {
    if (eventHandler_ != nullptr) {
        eventHandler_->onAdvertReceived(senderHash, timestamp);
    }
}
void PacketRouter::notifyDiscoveryRequest(const DiscoverRequest& request, int8_t snr) {
    if (eventHandler_ != nullptr) {
        eventHandler_->onDiscoveryRequest(request, snr);
    }
}
void PacketRouter::notifyPacketForward(const Packet& packet, uint32_t delayMs, uint8_t priority) {
    if (eventHandler_ != nullptr) {
        eventHandler_->onPacketForward(packet, delayMs, priority);
    }
}
void PacketRouter::notifyPacketDroppedDuplicate(const Packet& packet) {
    if (eventHandler_ != nullptr) {
        eventHandler_->onPacketDroppedDuplicate(packet);
    }
}
void PacketRouter::processAdvert(const Packet& packet) {
    uint32_t timestamp = 0;
    uint8_t senderHash = 0;
    auto status = extractAdvertTimestamp(packet.payload, packet.payloadLen, timestamp, senderHash);
    if (status.isOk()) {
        notifyAdvertReceived(senderHash, timestamp);
    }
}
void PacketRouter::processControlRequest(const Packet& packet, int8_t snr, bool isDuplicate) {
    if (isDuplicate) {
        notifyPacketDroppedDuplicate(packet);
        return;
    }
    DiscoverRequest discReq;
    if (parseDiscoverRequest(packet.payload, packet.payloadLen, discReq).isOk()) {
        notifyDiscoveryRequest(discReq, snr);
    }
}
void PacketRouter::processForwardable(Packet& packet, const RxPacket& rxInfo, bool isDuplicate) {
    (void)rxInfo;
    if (isDuplicate) {
        notifyPacketDroppedDuplicate(packet);
        return;
    }
    auto action = repeater_.shouldForward(packet);
    if (action != RepeaterAction::Forward) {
        return;
    }
    if (packet.isFloodRoute()) {
        if (!repeater_.appendSelfHash(packet, selfHash_).isOk()) {
            return;
        }
    } else if (packet.getPayloadType() == PayloadType::Trace) {
        repeater_.appendSnrToPath(packet);
    } else {
        repeater_.removeSelfFromPath(packet);
    }
    uint32_t delayMs = repeater_.calculateRetransmitDelay(packet, BASE_AIRTIME_MS);
    uint8_t priority = repeater_.getPriority(packet);
    notifyPacketForward(packet, delayMs, priority);
}
}
