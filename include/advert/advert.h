#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "core/config.h"
#include "core/result.h"
#include "crypto/crypto_types.h"
#include "packet/packet.h"
#include "hal/i_crypto.h"
namespace MiniCore {
using Config::MAX_ADVERT_DATA_SIZE;
using Config::SIGNATURE_SIZE;
using Config::ADV_TYPE_REPEATER;
using Config::ADV_NAME_MASK;
using Config::ADVERT_INTERVAL_HOURS;
using Config::ADVERT_STATE_STORAGE_SIZE;
constexpr uint32_t ADVERT_INTERVAL_SECONDS = ADVERT_INTERVAL_HOURS * 60 * 60;
[[nodiscard]] inline uint8_t encodeAdvertData(uint8_t nodeType, const char* name, uint8_t* buffer) {
    uint8_t flags = nodeType & 0x0F;
    size_t i = 1;
        if (name != nullptr && name[0] != '\0') {
        flags |= ADV_NAME_MASK;
        const char* src = name;
        while (*src != '\0' && i < MAX_ADVERT_DATA_SIZE) {
            buffer[i++] = static_cast<uint8_t>(*src++);
        }
    }
        buffer[0] = flags;
    return static_cast<uint8_t>(i);
}
[[nodiscard]] inline Status createAdvertPacket(
    const LocalIdentity& identity,
    uint32_t timestamp,
    const char* nodeName,
    ICrypto& crypto,
    Packet& packet
) {
    packet.header = Packet::makeHeader(RouteType::Flood, PayloadType::Advert);
    packet.pathLen = 0;
        size_t len = 0;
        std::memcpy(&packet.payload[len], identity.publicKey.bytes, PUB_KEY_SIZE);
    len += PUB_KEY_SIZE;
        std::memcpy(&packet.payload[len], &timestamp, sizeof(timestamp));
    len += sizeof(timestamp);
        uint8_t* signature = &packet.payload[len];
    len += SIGNATURE_SIZE;
        uint8_t appData[MAX_ADVERT_DATA_SIZE];
    uint8_t appDataLen = encodeAdvertData(ADV_TYPE_REPEATER, nodeName, appData);
    std::memcpy(&packet.payload[len], appData, appDataLen);
    len += appDataLen;
        packet.payloadLen = static_cast<uint8_t>(len);
        uint8_t message[PUB_KEY_SIZE + 4 + MAX_ADVERT_DATA_SIZE];
    size_t msgLen = 0;
    std::memcpy(&message[msgLen], identity.publicKey.bytes, PUB_KEY_SIZE);
    msgLen += PUB_KEY_SIZE;
    std::memcpy(&message[msgLen], &timestamp, sizeof(timestamp));
    msgLen += sizeof(timestamp);
    std::memcpy(&message[msgLen], appData, appDataLen);
    msgLen += appDataLen;
        return crypto.sign(signature, message, msgLen, identity.publicKey, identity.privateKey);
}
class AdvertScheduler {
public:
    [[nodiscard]] bool isDue(uint32_t currentTime, bool hasTimeSync) const {
        if (!hasTimeSync) {
            return false;
        }
        if (lastAdvertTime_ == 0) {
            return true;
        }
        if (currentTime <= lastAdvertTime_) {
            return false;
        }
        return (currentTime >= lastAdvertTime_ + ADVERT_INTERVAL_SECONDS);
    }
        void markSent(uint32_t timestamp) {
        lastAdvertTime_ = timestamp;
    }
        void setLastAdvertTime(uint32_t timestamp) {
        lastAdvertTime_ = timestamp;
    }
        [[nodiscard]] uint32_t lastAdvertTime() const {
        return lastAdvertTime_;
    }
        void saveTo(uint8_t* buffer) const {
        std::memcpy(buffer, &lastAdvertTime_, sizeof(lastAdvertTime_));
    }
        void loadFrom(const uint8_t* buffer) {
        uint32_t value;
        std::memcpy(&value, buffer, sizeof(value));
        if (value == 0xFFFFFFFF) {
            lastAdvertTime_ = 0;
        } else {
            lastAdvertTime_ = value;
        }
    }
    private:
    uint32_t lastAdvertTime_{0};
};
}
