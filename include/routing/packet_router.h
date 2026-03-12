#pragma once
#include <cstdint>
#include "core/config.h"
#include "packet/packet.h"
#include "packet/packet_history.h"
#include "routing/repeater.h"
#include "discovery/discovery.h"
#include "hal/i_radio.h"
namespace MiniCore {
class IPacketRouterEvents {
public:
    virtual ~IPacketRouterEvents() = default;
    virtual void onAdvertReceived(uint8_t senderHash, uint32_t timestamp) = 0;
    virtual void onDiscoveryRequest(const DiscoverRequest& request, int8_t snr) = 0;
    virtual void onPacketForward(const Packet& packet, uint32_t delayMs, uint8_t priority) = 0;
    virtual void onPacketDroppedDuplicate(const Packet& packet) = 0;
};
class PacketRouter {
public:
    explicit PacketRouter(Repeater& repeater);
    void setEventHandler(IPacketRouterEvents* handler) { eventHandler_ = handler; }
    void setSelfHash(uint8_t hash);
    [[nodiscard]] uint8_t selfHash() const { return selfHash_; }
    void processPacket(Packet& packet, const RxPacket& rxInfo);
private:
    void processAdvert(const Packet& packet);
    void processControlRequest(const Packet& packet, int8_t snr, bool isDuplicate);
    void processForwardable(Packet& packet, const RxPacket& rxInfo, bool isDuplicate);
    [[nodiscard]] bool checkAndMarkSeen(const Packet& packet);
    void notifyAdvertReceived(uint8_t senderHash, uint32_t timestamp);
    void notifyDiscoveryRequest(const DiscoverRequest& request, int8_t snr);
    void notifyPacketForward(const Packet& packet, uint32_t delayMs, uint8_t priority);
    void notifyPacketDroppedDuplicate(const Packet& packet);
    Repeater& repeater_;
    PacketHistory& history_;
    IPacketRouterEvents* eventHandler_{nullptr};
    uint8_t selfHash_{0};
};
}
