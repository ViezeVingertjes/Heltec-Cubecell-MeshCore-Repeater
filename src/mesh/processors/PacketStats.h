#pragma once

#include "../PacketDispatcher.h"
#include "../../core/Logger.h"
#include "../../core/Config.h"

namespace MeshCore {

class PacketStats : public IPacketProcessor {
public:
    PacketStats() = default;
    ~PacketStats() override = default;
    
    ProcessResult processPacket(const PacketEvent& event, ProcessingContext& ctx) override;
    const char* getName() const override { return "PacketStats"; }
    uint8_t getPriority() const override { return 40; }
    
    void printStats();
    void resetStats();

private:
    uint32_t totalPackets = 0;
    uint32_t advertPackets = 0;
    int16_t minRssi = 0;
    int16_t maxRssi = -200;
    int32_t rssiSum = 0;
};

}
