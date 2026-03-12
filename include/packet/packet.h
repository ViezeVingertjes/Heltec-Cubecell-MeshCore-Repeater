#pragma once
#include <cstdint>
#include <cstddef>
#include "core/config.h"
#include "core/result.h"
namespace MiniCore {
using Config::MAX_PACKET_PAYLOAD;
using Config::MAX_PATH_SIZE;
using Config::MAX_MTU_SIZE;
using Config::MIN_PACKET_SIZE;
constexpr uint8_t HEADER_ROUTE_MASK = 0x03;
constexpr uint8_t HEADER_TYPE_SHIFT = 2;
constexpr uint8_t HEADER_TYPE_MASK = 0x0F;
constexpr uint8_t HEADER_VER_SHIFT = 6;
constexpr uint8_t HEADER_VER_MASK = 0x03;
constexpr size_t TRANSPORT_CODES_SIZE = 4;
constexpr size_t ROUTE_PAYLOAD_LABEL_SIZE = 16;
constexpr size_t PACKET_STATS_LABEL_SIZE = 32;
enum class RouteType : uint8_t {
    TransportFlood = 0x00,
    Flood = 0x01,
    Direct = 0x02,
    TransportDirect = 0x03
};
enum class PayloadType : uint8_t {
    Request = 0x00,
    Response = 0x01,
    TextMessage = 0x02,
    Ack = 0x03,
    Advert = 0x04,
    GroupText = 0x05,
    GroupData = 0x06,
    AnonRequest = 0x07,
    Path = 0x08,
    Trace = 0x09,
    Multipart = 0x0A,
    Control = 0x0B,
    RawCustom = 0x0F
};
enum class PayloadVersion : uint8_t {
    V1 = 0x00,
    V2 = 0x01,
    V3 = 0x02,
    V4 = 0x03
};
struct Packet {
    uint8_t header;
    uint16_t transportCodes[2];
    uint8_t pathLen;
    uint8_t path[MAX_PATH_SIZE];
    uint16_t payloadLen;
    uint8_t payload[MAX_PACKET_PAYLOAD];
    int8_t snr;
    Packet();
    [[nodiscard]] RouteType getRouteType() const;
    [[nodiscard]] PayloadType getPayloadType() const;
    [[nodiscard]] PayloadVersion getPayloadVersion() const;
    [[nodiscard]] bool hasTransportCodes() const;
    [[nodiscard]] bool isFloodRoute() const;
    [[nodiscard]] bool isDirectRoute() const;
    [[nodiscard]] uint16_t getRawLength() const;
    static constexpr uint8_t makeHeader(RouteType route, PayloadType type, 
                                        PayloadVersion version = PayloadVersion::V1) {
        return static_cast<uint8_t>(route) |
               (static_cast<uint8_t>(type) << HEADER_TYPE_SHIFT) |
               (static_cast<uint8_t>(version) << HEADER_VER_SHIFT);
    }
};
[[nodiscard]] Status decodePacket(const uint8_t* src, uint8_t len, Packet& packet);
[[nodiscard]] Status encodePacket(const Packet& packet, uint8_t* dest, uint8_t& destLen);
constexpr uint8_t CONTROL_REQUEST_FLAG = 0x80;
[[nodiscard]] inline bool isValidControlRequest(const Packet& packet) {
    if (packet.getPayloadType() != PayloadType::Control) {
        return false;
    }
    if (!packet.isDirectRoute()) {
        return false;
    }
    if (packet.pathLen != 0) {
        return false;
    }
    if (packet.payloadLen == 0) {
        return false;
    }
    return (packet.payload[0] & CONTROL_REQUEST_FLAG) != 0;
}
const char* payloadTypeName(PayloadType type);
const char* routeTypeName(RouteType type);
void formatPacketStats(const Packet& packet, char* buffer, size_t bufferSize);
void formatRouteAndPayloadLabel(const Packet& packet, char* buffer, size_t bufferSize);
struct PartialPacketInfo {
    bool hasHeader;
    RouteType routeType;
    PayloadType payloadType;
    ErrorCode decodeError;
    uint8_t pathLen;
    bool hasTransportCodes;
};
[[nodiscard]] PartialPacketInfo extractPartialPacketInfo(const uint8_t* data, uint8_t len, Status decodeStatus);
}
