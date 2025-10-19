#pragma once

#include "../PacketDispatcher.h"
#include "../../core/Logger.h"

namespace MeshCore {

class PacketLogger : public IPacketProcessor {
public:
    PacketLogger() = default;
    ~PacketLogger() override = default;
    
    ProcessResult processPacket(const PacketEvent& event, ProcessingContext& ctx) override;
    const char* getName() const override { return "PacketLogger"; }
    uint8_t getPriority() const override { return 30; }
};

}
