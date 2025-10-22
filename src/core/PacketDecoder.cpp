#include "PacketDecoder.h"
#include "Logger.h"
#include "PacketValidator.h"
#include <string.h>

namespace MeshCore {

bool PacketDecoder::decode(const uint8_t *raw, uint16_t length,
                           DecodedPacket &packet) {
  if (length < MIN_PACKET_SIZE)
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
    if (idx + TRANSPORT_CODES_TOTAL_SIZE > length)
      return false;
    memcpy(&packet.transportCodes[0], &raw[idx], TRANSPORT_CODE_SIZE);
    idx += TRANSPORT_CODE_SIZE;
    memcpy(&packet.transportCodes[1], &raw[idx], TRANSPORT_CODE_SIZE);
    idx += TRANSPORT_CODE_SIZE;
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
  // Validate packet before encoding
  auto validation = PacketValidator::validate(packet, ValidationLevel::BASIC);
  if (validation.isError()) {
    LOG_ERROR_FMT("Cannot encode invalid packet: %s",
                  errorCodeToString(validation.error));
    return 0;
  }

  uint16_t idx = 0;

  // Calculate total size needed
  uint16_t requiredSize = 1; // header
  if (packet.hasTransportCodes)
    requiredSize += TRANSPORT_CODES_TOTAL_SIZE;
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
    memcpy(&raw[idx], &packet.transportCodes[0], TRANSPORT_CODE_SIZE);
    idx += TRANSPORT_CODE_SIZE;
    memcpy(&raw[idx], &packet.transportCodes[1], TRANSPORT_CODE_SIZE);
    idx += TRANSPORT_CODE_SIZE;
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
  if (length < ADVERT_MIN_PAYLOAD_SIZE)
    return false;

  uint16_t idx = 0;

  // Skip fixed header fields (ID + timestamp + key)
  if (idx + ADVERT_ID_SIZE + ADVERT_TIMESTAMP_SIZE + ADVERT_KEY_SIZE > length)
    return false;
  idx += ADVERT_ID_SIZE + ADVERT_TIMESTAMP_SIZE + ADVERT_KEY_SIZE;

  // Parse flags and determine what fields are present
  if (!parseAdvertFlags(payload, idx, length, packet))
    return false;

  // Parse optional fields based on flags
  if (packet.hasLocation && !parseAdvertLocation(payload, idx, length, packet))
    return false;

  return true;
}

bool PacketDecoder::parseAdvertFlags(const uint8_t *payload, uint16_t &idx,
                                     uint16_t length, DecodedPacket &packet) {
  if (idx >= length)
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

  if (!parseAdvertFeatures(payload, idx, length, hasFeat1, hasFeat2, packet))
    return false;

  if (hasName && !parseAdvertName(payload, idx, length, packet))
    return false;

  return true;
}

bool PacketDecoder::parseAdvertLocation(const uint8_t *payload, uint16_t &idx,
                                        uint16_t length, DecodedPacket &packet) {
  if (idx + ADVERT_LOCATION_SIZE > length)
    return false;

  int32_t lat, lon;
  memcpy(&lat, &payload[idx], sizeof(int32_t));
  idx += sizeof(int32_t);
  memcpy(&lon, &payload[idx], sizeof(int32_t));
  idx += sizeof(int32_t);
  
  packet.latitude = static_cast<double>(lat) / LOCATION_SCALE_FACTOR;
  packet.longitude = static_cast<double>(lon) / LOCATION_SCALE_FACTOR;
  
  int32_t latWhole, latFrac, lonWhole, lonFrac;
  formatLocation(packet.latitude, packet.longitude, latWhole, latFrac, lonWhole, lonFrac);
  LOG_INFO_FMT("Parsed location: %d.%06d, %d.%06d (raw: %d, %d)",
               latWhole, latFrac, lonWhole, lonFrac, lat, lon);
  return true;
}

bool PacketDecoder::parseAdvertFeatures(const uint8_t *payload, uint16_t &idx,
                                        uint16_t length, bool hasFeat1,
                                        bool hasFeat2, DecodedPacket &packet) {
  if (hasFeat1) {
    if (idx + ADVERT_FEATURE_SIZE > length)
      return false;
    memcpy(&packet.advertFeat1, &payload[idx], ADVERT_FEATURE_SIZE);
    idx += ADVERT_FEATURE_SIZE;
  }

  if (hasFeat2) {
    if (idx + ADVERT_FEATURE_SIZE > length)
      return false;
    memcpy(&packet.advertFeat2, &payload[idx], ADVERT_FEATURE_SIZE);
    idx += ADVERT_FEATURE_SIZE;
  }

  return true;
}

bool PacketDecoder::parseAdvertName(const uint8_t *payload, uint16_t &idx,
                                    uint16_t length, DecodedPacket &packet) {
  uint8_t nameLen = 0;
  while (idx < length && payload[idx] != 0 &&
         nameLen < sizeof(packet.advertName) - 1) {
    packet.advertName[nameLen++] = payload[idx++];
  }
  packet.advertName[nameLen] = '\0';
  
  // Skip null terminator if present
  if (idx < length && payload[idx] == 0)
    idx++;

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

void PacketDecoder::formatLocation(double latitude, double longitude,
                                   int32_t &latWhole, int32_t &latFrac,
                                   int32_t &lonWhole, int32_t &lonFrac) {
  int32_t latInt = static_cast<int32_t>(latitude * LOCATION_SCALE_FACTOR);
  int32_t lonInt = static_cast<int32_t>(longitude * LOCATION_SCALE_FACTOR);
  
  latWhole = latInt / LOCATION_SCALE_FACTOR;
  latFrac = latInt % LOCATION_SCALE_FACTOR;
  lonWhole = lonInt / LOCATION_SCALE_FACTOR;
  lonFrac = lonInt % LOCATION_SCALE_FACTOR;
}

} // namespace MeshCore
