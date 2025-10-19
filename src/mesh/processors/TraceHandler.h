#pragma once

#include "../PacketDispatcher.h"
#include "../../core/Logger.h"
#include "../../core/Config.h"

namespace MeshCore {

class TraceHandler : public IPacketProcessor {
public:
    TraceHandler() : tracesHandled(0) {}
    ~TraceHandler() override = default;
    
    ProcessResult processPacket(const PacketEvent& event, ProcessingContext& ctx) override;
    const char* getName() const override { return "TraceHandler"; }
    uint8_t getPriority() const override { return 45; }
    
    uint32_t getTracesHandled() const { return tracesHandled; }

private:
    uint32_t tracesHandled;
    
    bool isNodeHashMatch(uint8_t hash) const;
    bool shouldForwardTrace(const DecodedPacket& packet, const ProcessingContext& ctx) const;
    bool appendSnrAndForward(DecodedPacket& packet, int8_t snr);
    void handleTraceComplete(const DecodedPacket& packet);
};

}

