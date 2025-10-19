#include "PacketDecoder.h"
#include "Logger.h"
#include <string.h>

namespace MeshCore {

bool PacketDecoder::decode(const uint8_t *raw, uint16_t length,
                           DecodedPacket &packet) {
  if (length < 2)
    return false;

  memset(&packet, 0, sizeof(packet));

  uint16_t idx = 0;

  packet.header = raw[idx++];
  packet.routeType = static_cast<RouteType>(packet.header & PH_ROUTE_MASK);
  packet.payloadType =
      static_cast<PayloadType>((packet.header >> PH_TYPE_SHIFT) & PH_TYPE_MASK);
  packet.payloadVersion = (packet.header >> PH_VER_SHIFT) & PH_VER_MASK;
  packet.hasTransportCodes = (packet.routeType == RouteType::TRANSPORT_FLOOD ||
                              packet.routeType == RouteType::TRANSPORT_DIRECT);

  if (packet.hasTransportCodes) {
    if (idx + 4 > length)
      return false;
    memcpy(&packet.transportCodes[0], &raw[idx], 2);
    idx += 2;
    memcpy(&packet.transportCodes[1], &raw[idx], 2);
    idx += 2;
  }

  if (idx >= length)
    return false;
  packet.pathLength = raw[idx++];

  if (packet.pathLength > MAX_PATH_SIZE)
    return false;
  if (idx + packet.pathLength > length)
    return false;
  memcpy(packet.path, &raw[idx], packet.pathLength);
  idx += packet.pathLength;

  if (idx > length)
    return false;
  packet.payloadLength = length - idx;

  if (packet.payloadLength > MAX_PACKET_PAYLOAD)
    return false;
  memcpy(packet.payload, &raw[idx], packet.payloadLength);

  packet.isAdvertDecoded = false;
  if (packet.payloadType == PayloadType::ADVERT && packet.payloadLength > 0) {
    packet.isAdvertDecoded =
        decodeAdvertPayload(packet.payload, packet.payloadLength, packet);
  }

  return true;
}

uint16_t PacketDecoder::encode(const DecodedPacket &packet, uint8_t *raw,
                               uint16_t maxLength) {
  uint16_t idx = 0;

  // Calculate total size needed
  uint16_t requiredSize = 1; // header
  if (packet.hasTransportCodes)
    requiredSize += 4;
  requiredSize += 1; // path length
  requiredSize += packet.pathLength;
  requiredSize += packet.payloadLength;

  if (requiredSize > maxLength) {
    LOG_ERROR_FMT("Encode buffer too small: need %d, have %d", requiredSize,
                  maxLength);
    return 0;
  }

  LOG_INFO_FMT("Encoding packet: path_len=%d, payload_len=%d",
               packet.pathLength, packet.payloadLength);

  // Write header
  raw[idx++] = packet.header;

  // Write transport codes if present
  if (packet.hasTransportCodes) {
    memcpy(&raw[idx], &packet.transportCodes[0], 2);
    idx += 2;
    memcpy(&raw[idx], &packet.transportCodes[1], 2);
    idx += 2;
  }

  // Write path length
  raw[idx++] = packet.pathLength;

  // Write path
  if (packet.pathLength > 0) {
    memcpy(&raw[idx], packet.path, packet.pathLength);
    if (packet.pathLength >= 2) {
      LOG_INFO_FMT("Encoded path (%d bytes): last 2 bytes = 0x%02X 0x%02X",
                   packet.pathLength, packet.path[packet.pathLength - 2],
                   packet.path[packet.pathLength - 1]);
    } else {
      LOG_INFO_FMT("Encoded path (%d bytes)", packet.pathLength);
    }
    idx += packet.pathLength;
  }

  // Write payload
  if (packet.payloadLength > 0) {
    memcpy(&raw[idx], packet.payload, packet.payloadLength);
    idx += packet.payloadLength;
  }

  LOG_INFO_FMT("Encoded packet: total %d bytes", idx);

  return idx;
}

bool PacketDecoder::decodeAdvertPayload(const uint8_t *payload, uint16_t length,
                                        DecodedPacket &packet) {
  if (length < 100)
    return false;

  uint16_t idx = 0;

  if (idx + 32 > length)
    return false;
  idx += 32;

  if (idx + 4 > length)
    return false;
  idx += 4;

  if (idx + 64 > length)
    return false;
  idx += 64;

  uint16_t appdataLen = length - idx;
  if (appdataLen < 1)
    return false;

  uint8_t flags = payload[idx++];

  packet.advertType = static_cast<AdvertType>(flags & 0x0F);
  packet.hasLocation = (flags & ADV_LATLON_MASK) != 0;
  bool hasFeat1 = (flags & ADV_FEAT1_MASK) != 0;
  bool hasFeat2 = (flags & ADV_FEAT2_MASK) != 0;
  bool hasName = (flags & ADV_NAME_MASK) != 0;

  LOG_INFO_FMT("Advert flags: 0x%02X, type=%d, hasLocation=%d, hasFeat1=%d, "
               "hasFeat2=%d, hasName=%d",
               flags, packet.advertType, packet.hasLocation, hasFeat1, hasFeat2,
               hasName);

  if (packet.hasLocation) {
    if (idx + 8 > length)
      return false;
    int32_t lat, lon;
    memcpy(&lat, &payload[idx], 4);
    idx += 4;
    memcpy(&lon, &payload[idx], 4);
    idx += 4;
    packet.latitude = lat / 1000000.0;
    packet.longitude = lon / 1000000.0;
    LOG_INFO_FMT("Raw lat=%d, lon=%d -> lat=%d.%06d, lon=%d.%06d", lat, lon,
                 lat / 1000000, lat % 1000000, lon / 1000000, lon % 1000000);
  } else {
    packet.latitude = 0.0;
    packet.longitude = 0.0;
  }

  if (hasFeat1) {
    if (idx + 2 > length)
      return false;
    memcpy(&packet.advertFeat1, &payload[idx], 2);
    idx += 2;
  }

  if (hasFeat2) {
    if (idx + 2 > length)
      return false;
    memcpy(&packet.advertFeat2, &payload[idx], 2);
    idx += 2;
  }

  if (hasName) {
    uint8_t nameLen = 0;
    while (idx < length && payload[idx] != 0 &&
           nameLen < sizeof(packet.advertName) - 1) {
      packet.advertName[nameLen++] = payload[idx++];
    }
    packet.advertName[nameLen] = '\0';
    if (idx < length && payload[idx] == 0)
      idx++;
  }

  return true;
}

const char *PacketDecoder::routeTypeToString(RouteType type) {
  switch (type) {
  case RouteType::TRANSPORT_FLOOD:
    return "TransportFlood";
  case RouteType::FLOOD:
    return "Flood";
  case RouteType::DIRECT:
    return "Direct";
  case RouteType::TRANSPORT_DIRECT:
    return "TransportDirect";
  default:
    return "Unknown";
  }
}

const char *PacketDecoder::payloadTypeToString(PayloadType type) {
  switch (type) {
  case PayloadType::REQ:
    return "Request";
  case PayloadType::RESPONSE:
    return "Response";
  case PayloadType::TXT_MSG:
    return "TextMsg";
  case PayloadType::ACK:
    return "Ack";
  case PayloadType::ADVERT:
    return "Advert";
  case PayloadType::GRP_TXT:
    return "GroupText";
  case PayloadType::GRP_DATA:
    return "GroupData";
  case PayloadType::ANON_REQ:
    return "AnonReq";
  case PayloadType::PATH:
    return "Path";
  case PayloadType::TRACE:
    return "Trace";
  case PayloadType::MULTIPART:
    return "Multipart";
  case PayloadType::RAW_CUSTOM:
    return "RawCustom";
  default:
    return "Unknown";
  }
}

const char *PacketDecoder::advertTypeToString(AdvertType type) {
  switch (type) {
  case AdvertType::NONE:
    return "None";
  case AdvertType::CHAT:
    return "Chat";
  case AdvertType::REPEATER:
    return "Repeater";
  case AdvertType::ROOM:
    return "Room";
  case AdvertType::SENSOR:
    return "Sensor";
  default:
    return "Unknown";
  }
}

} // namespace MeshCore
