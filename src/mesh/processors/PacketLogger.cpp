#include "PacketLogger.h"
#include "../../core/PacketDecoder.h"

namespace MeshCore {

ProcessResult PacketLogger::processPacket(const PacketEvent &event,
                                          ProcessingContext &ctx) {
  LOG_INFO_FMT("Decoded packet: %s/%s v%d",
               PacketDecoder::routeTypeToString(event.packet.routeType),
               PacketDecoder::payloadTypeToString(event.packet.payloadType),
               event.packet.payloadVersion);

  if (event.packet.hasTransportCodes) {
    LOG_DEBUG_FMT("Transport codes: [%d, %d]", event.packet.transportCodes[0],
                  event.packet.transportCodes[1]);
  }

  LOG_DEBUG_FMT("Path length: %d, Payload length: %d", event.packet.pathLength,
                event.packet.payloadLength);

  if (event.packet.isAdvertDecoded) {
    LOG_INFO_FMT("Advertisement: %s",
                 PacketDecoder::advertTypeToString(event.packet.advertType));

    if (event.packet.advertName[0] != '\0') {
      LOG_INFO_FMT("Name: %s", event.packet.advertName);
    }

    if (event.packet.hasLocation) {
      int32_t latWhole, latFrac, lonWhole, lonFrac;
      PacketDecoder::formatLocation(event.packet.latitude, event.packet.longitude,
                                   latWhole, latFrac, lonWhole, lonFrac);
      LOG_INFO_FMT("Location: %d.%06d, %d.%06d", latWhole, latFrac, lonWhole, lonFrac);
    }

    if (event.packet.advertFeat1 != 0 || event.packet.advertFeat2 != 0) {
      LOG_DEBUG_FMT("Features: feat1=%d, feat2=%d", event.packet.advertFeat1,
                    event.packet.advertFeat2);
    }
  }

  return ProcessResult::CONTINUE;
}

} // namespace MeshCore
