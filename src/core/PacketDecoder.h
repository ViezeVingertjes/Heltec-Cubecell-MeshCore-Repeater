#pragma once

#include <Arduino.h>

namespace MeshCore {

constexpr uint8_t PH_ROUTE_MASK = 0x03;
constexpr uint8_t PH_TYPE_SHIFT = 2;
constexpr uint8_t PH_TYPE_MASK = 0x0F;
constexpr uint8_t PH_VER_SHIFT = 6;
constexpr uint8_t PH_VER_MASK = 0x03;

enum class RouteType : uint8_t {
    TRANSPORT_FLOOD = 0x00,
    FLOOD = 0x01,
    DIRECT = 0x02,
    TRANSPORT_DIRECT = 0x03
};

enum class PayloadType : uint8_t {
    REQ = 0x00,
    RESPONSE = 0x01,
    TXT_MSG = 0x02,
    ACK = 0x03,
    ADVERT = 0x04,
    GRP_TXT = 0x05,
    GRP_DATA = 0x06,
    ANON_REQ = 0x07,
    PATH = 0x08,
    TRACE = 0x09,
    MULTIPART = 0x0A,
    RAW_CUSTOM = 0x0F
};

enum class AdvertType : uint8_t {
    NONE = 0,
    CHAT = 1,
    REPEATER = 2,
    ROOM = 3,
    SENSOR = 4
};

constexpr uint8_t ADV_LATLON_MASK = 0x10;
constexpr uint8_t ADV_FEAT1_MASK = 0x20;
constexpr uint8_t ADV_FEAT2_MASK = 0x40;
constexpr uint8_t ADV_NAME_MASK = 0x80;

constexpr uint8_t MAX_PACKET_PAYLOAD = 184;
constexpr uint8_t MAX_PATH_SIZE = 64;
constexpr uint8_t MAX_ADVERT_DATA_SIZE = 32;

struct DecodedPacket {
    uint8_t header;
    RouteType routeType;
    PayloadType payloadType;
    uint8_t payloadVersion;
    bool hasTransportCodes;
    uint16_t transportCodes[2];
    uint8_t pathLength;
    uint8_t path[MAX_PATH_SIZE];
    uint16_t payloadLength;
    uint8_t payload[MAX_PACKET_PAYLOAD];
    
    bool isAdvertDecoded;
    AdvertType advertType;
    char advertName[MAX_ADVERT_DATA_SIZE];
    bool hasLocation;
    double latitude;
    double longitude;
    uint16_t advertFeat1;
    uint16_t advertFeat2;
};

class PacketDecoder {
public:
    static bool decode(const uint8_t* raw, uint16_t length, DecodedPacket& packet);
    static uint16_t encode(const DecodedPacket& packet, uint8_t* raw, uint16_t maxLength);
    static const char* routeTypeToString(RouteType type);
    static const char* payloadTypeToString(PayloadType type);
    static const char* advertTypeToString(AdvertType type);
    
private:
    static bool decodeAdvertPayload(const uint8_t* payload, uint16_t length, DecodedPacket& packet);
};

}
