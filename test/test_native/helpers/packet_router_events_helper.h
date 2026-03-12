#pragma once
#include "routing/packet_router.h"
namespace MiniCore {

class TestPacketRouterEvents : public IPacketRouterEvents {
public:
    bool advertReceivedCalled{false};
    uint32_t lastAdvertTimestamp{0};
    uint8_t lastAdvertSenderHash{0};
    bool discoveryRequestCalled{false};
    DiscoverRequest lastDiscoverRequest{};
    int8_t lastSnr{0};
    bool forwardPacketCalled{false};
    Packet lastForwardPacket{};
    uint32_t lastForwardDelay{0};
    uint8_t lastForwardPriority{0};
    bool duplicateDroppedCalled{false};
    Packet lastDroppedPacket{};
    void onAdvertReceived(uint8_t senderHash, uint32_t timestamp) override {
        advertReceivedCalled = true;
        lastAdvertSenderHash = senderHash;
        lastAdvertTimestamp = timestamp;
    }
    void onDiscoveryRequest(const DiscoverRequest& request, int8_t snr) override {
        discoveryRequestCalled = true;
        lastDiscoverRequest = request;
        lastSnr = snr;
    }
    void onPacketForward(const Packet& packet, uint32_t delayMs, uint8_t priority) override {
        forwardPacketCalled = true;
        lastForwardPacket = packet;
        lastForwardDelay = delayMs;
        lastForwardPriority = priority;
    }
    void onPacketDroppedDuplicate(const Packet& packet) override {
        duplicateDroppedCalled = true;
        lastDroppedPacket = packet;
    }
    void reset() {
        advertReceivedCalled = false;
        lastAdvertTimestamp = 0;
        lastAdvertSenderHash = 0;
        discoveryRequestCalled = false;
        lastDiscoverRequest = {};
        lastSnr = 0;
        forwardPacketCalled = false;
        lastForwardPacket = {};
        lastForwardDelay = 0;
        lastForwardPriority = 0;
        duplicateDroppedCalled = false;
        lastDroppedPacket = {};
    }
};
}
