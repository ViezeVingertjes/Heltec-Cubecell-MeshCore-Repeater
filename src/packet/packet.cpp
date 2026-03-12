#include "packet/packet.h"
#include <cstring>
#include <cstdio>
namespace MiniCore {
constexpr uint8_t HEADER_SIZE = 1;
constexpr uint8_t PATH_LEN_SIZE = 1;
constexpr uint8_t TRANSPORT_CODE_SIZE = 2;
Packet::Packet()
    : header(0)
    , transportCodes{0, 0}
    , pathLen(0)
    , path{}
    , payloadLen(0)
    , payload{}
    , snr(0)
{
}
RouteType Packet::getRouteType() const {
    return static_cast<RouteType>(header & HEADER_ROUTE_MASK);
}
PayloadType Packet::getPayloadType() const {
    return static_cast<PayloadType>((header >> HEADER_TYPE_SHIFT) & HEADER_TYPE_MASK);
}
PayloadVersion Packet::getPayloadVersion() const {
    return static_cast<PayloadVersion>((header >> HEADER_VER_SHIFT) & HEADER_VER_MASK);
}
bool Packet::hasTransportCodes() const {
    const auto route = getRouteType();
    return route == RouteType::TransportFlood || route == RouteType::TransportDirect;
}
bool Packet::isFloodRoute() const {
    const auto route = getRouteType();
    return route == RouteType::Flood || route == RouteType::TransportFlood;
}
bool Packet::isDirectRoute() const {
    const auto route = getRouteType();
    return route == RouteType::Direct || route == RouteType::TransportDirect;
}
uint16_t Packet::getRawLength() const {
    uint16_t len = HEADER_SIZE + PATH_LEN_SIZE;
    if (hasTransportCodes()) {
        len += TRANSPORT_CODES_SIZE;
    }
    len += pathLen;
    len += payloadLen;
    return len;
}

Status decodePacket(const uint8_t* src, uint8_t len, Packet& packet) {

    if (src == nullptr || len < MIN_PACKET_SIZE) {
        return ErrorCode::InvalidParameter;
    }
    uint8_t idx = 0;
    packet.header = src[idx++];
    if (packet.hasTransportCodes()) {
        if (len < idx + TRANSPORT_CODES_SIZE) {
            return ErrorCode::InvalidParameter;
        }
        std::memcpy(&packet.transportCodes[0], &src[idx], TRANSPORT_CODE_SIZE);
        idx += TRANSPORT_CODE_SIZE;
        std::memcpy(&packet.transportCodes[1], &src[idx], TRANSPORT_CODE_SIZE);
        idx += TRANSPORT_CODE_SIZE;
    } else {
        packet.transportCodes[0] = 0;
        packet.transportCodes[1] = 0;
    }
    if (idx >= len) {
        return ErrorCode::InvalidParameter;
    }
        packet.pathLen = src[idx++];
    if (packet.pathLen > Config::MAX_HOPS) {
        return ErrorCode::TooManyHops;
    }
    if (packet.pathLen > MAX_PATH_SIZE) {
        return ErrorCode::BufferOverflow;
    }
    if (idx + packet.pathLen > len) {
        return ErrorCode::InvalidParameter;
    }
    if (packet.pathLen > 0) {
        std::memcpy(packet.path, &src[idx], packet.pathLen);
        idx += packet.pathLen;
    }

    if (idx >= len) {
        return ErrorCode::InvalidParameter;
    }
        packet.payloadLen = len - idx;
        if (packet.payloadLen > MAX_PACKET_PAYLOAD) {
        return ErrorCode::BufferOverflow;
    }
    std::memcpy(packet.payload, &src[idx], packet.payloadLen);
    return Status();
}

Status encodePacket(const Packet& packet, uint8_t* dest, uint8_t& destLen) {
    if (dest == nullptr) {
        return ErrorCode::InvalidParameter;
    }
    if (packet.pathLen > MAX_PATH_SIZE) {
        return ErrorCode::BufferOverflow;
    }
    if (packet.payloadLen > MAX_PACKET_PAYLOAD) {
        return ErrorCode::BufferOverflow;
    }
    uint8_t idx = 0;
    dest[idx++] = packet.header;
    if (packet.hasTransportCodes()) {
        std::memcpy(&dest[idx], &packet.transportCodes[0], TRANSPORT_CODE_SIZE);
        idx += TRANSPORT_CODE_SIZE;
        std::memcpy(&dest[idx], &packet.transportCodes[1], TRANSPORT_CODE_SIZE);
        idx += TRANSPORT_CODE_SIZE;
    }
    dest[idx++] = packet.pathLen;
    if (packet.pathLen > 0) {
        std::memcpy(&dest[idx], packet.path, packet.pathLen);
        idx += packet.pathLen;
    }

    std::memcpy(&dest[idx], packet.payload, packet.payloadLen);
    idx += static_cast<uint8_t>(packet.payloadLen);
    destLen = idx;
    return Status();
}
const char* payloadTypeName(PayloadType type) {
    switch (type) {
        case PayloadType::Request:     return "REQ";
        case PayloadType::Response:    return "RSP";
        case PayloadType::TextMessage: return "TXT";
        case PayloadType::Ack:         return "ACK";
        case PayloadType::Advert:      return "ADV";
        case PayloadType::GroupText:   return "GRP";
        case PayloadType::GroupData:   return "GRD";
        case PayloadType::AnonRequest: return "ANO";
        case PayloadType::Path:        return "PTH";
        case PayloadType::Trace:       return "TRC";
        case PayloadType::Multipart:   return "MUL";
        case PayloadType::Control:     return "CTL";
        case PayloadType::RawCustom:   return "RAW";
        default:                       return "???";
    }
}
const char* routeTypeName(RouteType type) {
    switch (type) {
        case RouteType::Flood:          return "FLD";
        case RouteType::Direct:         return "DIR";
        case RouteType::TransportFlood: return "TFL";
        case RouteType::TransportDirect:return "TDR";
        default:                        return "???";
    }
}
void formatPacketStats(const Packet& packet, char* buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) {
        return;
    }
        std::snprintf(buffer, bufferSize, "%s:%s p:%u d:%u",
                  routeTypeName(packet.getRouteType()),
                  payloadTypeName(packet.getPayloadType()),
                  packet.pathLen,
                  packet.payloadLen);
}
void formatRouteAndPayloadLabel(const Packet& packet, char* buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) {
        return;
    }
    const char* routeTag = packet.isFloodRoute() ? "FLD" :
                          (packet.getPayloadType() == PayloadType::Trace ? "TRC" : "DIR");
    std::snprintf(buffer, bufferSize, "%s %s", routeTag, payloadTypeName(packet.getPayloadType()));
}
PartialPacketInfo extractPartialPacketInfo(const uint8_t* data, uint8_t len, Status decodeStatus) {
    PartialPacketInfo info{};
    info.decodeError = decodeStatus.isError() ? decodeStatus.error() : ErrorCode::None;
        if (data == nullptr || len == 0) {
        return info;
    }
        if (len >= 1) {
        info.hasHeader = true;
        uint8_t header = data[0];
        info.routeType = static_cast<RouteType>(header & HEADER_ROUTE_MASK);
        info.payloadType = static_cast<PayloadType>((header >> HEADER_TYPE_SHIFT) & HEADER_TYPE_MASK);
        info.hasTransportCodes = (info.routeType == RouteType::TransportFlood || 
                                  info.routeType == RouteType::TransportDirect);
                uint8_t idx = 1;
        if (info.hasTransportCodes && len >= idx + TRANSPORT_CODES_SIZE) {
            idx += TRANSPORT_CODES_SIZE;
        }
                if (len > idx) {
            info.pathLen = data[idx];
        }
    }
        return info;
}
}
