#include "PacketLogger.h"

namespace MeshCore {

ProcessResult PacketLogger::processPacket(const PacketEvent& event, ProcessingContext& ctx) {
    LOG_INFO_FMT("Decoded packet: %s/%s v%d", 
        PacketDecoder::routeTypeToString(event.packet.routeType),
        PacketDecoder::payloadTypeToString(event.packet.payloadType),
        event.packet.payloadVersion);
    
    if (event.packet.hasTransportCodes) {
        LOG_DEBUG_FMT("Transport codes: [%d, %d]", 
            event.packet.transportCodes[0], event.packet.transportCodes[1]);
    }
    
    LOG_DEBUG_FMT("Path length: %d, Payload length: %d", 
        event.packet.pathLength, event.packet.payloadLength);
    
    if (event.packet.isAdvertDecoded) {
        LOG_INFO_FMT("Advertisement: %s", 
            PacketDecoder::advertTypeToString(event.packet.advertType));
        
        if (event.packet.advertName[0] != '\0') {
            LOG_INFO_FMT("Name: %s", event.packet.advertName);
        }
        
        if (event.packet.hasLocation) {
            int32_t latInt = (int32_t)(event.packet.latitude * 1000000);
            int32_t lonInt = (int32_t)(event.packet.longitude * 1000000);
            LOG_INFO_FMT("Location: %d.%06d, %d.%06d", 
                latInt / 1000000, latInt % 1000000,
                lonInt / 1000000, lonInt % 1000000);
        }
        
        if (event.packet.advertFeat1 != 0 || event.packet.advertFeat2 != 0) {
            LOG_DEBUG_FMT("Features: feat1=%d, feat2=%d", 
                event.packet.advertFeat1, event.packet.advertFeat2);
        }
    }
    
    return ProcessResult::CONTINUE;
}

}
