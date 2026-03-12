#include "packet/packet_history.h"
#include <cstring>
extern "C" {
#include "sha256.h"
}
namespace MiniCore {
PacketHistory::PacketHistory()
    : hashes_{}
    , nextHashIdx_(0)
    , acks_{}
    , nextAckIdx_(0)
    , directDups_(0)
    , floodDups_(0)
{
    clearAll();
}
bool PacketHistory::hasSeen(const Packet& packet) {

    if (packet.getPayloadType() == PayloadType::Ack) {
        uint32_t ack;
        std::memcpy(&ack, packet.payload, 4);
                if (findAck(ack)) {

            if (packet.isDirectRoute()) {
                directDups_++;
            } else {
                floodDups_++;
            }
            return true;
        }
                addAck(ack);
        return false;
    }

    uint8_t hash[MAX_HASH_SIZE];
    calculateHash(packet, hash);
    if (findHash(hash)) {

        if (packet.isDirectRoute()) {
            directDups_++;
        } else {
            floodDups_++;
        }
        return true;
    }
    addHash(hash);
    return false;
}
void PacketHistory::clear(const Packet& packet) {
    if (packet.getPayloadType() == PayloadType::Ack) {
        uint32_t ack;
        std::memcpy(&ack, packet.payload, 4);
        clearAck(ack);
    } else {
        uint8_t hash[MAX_HASH_SIZE];
        calculateHash(packet, hash);
        clearHash(hash);
    }
}
bool PacketHistory::contains(const Packet& packet) const {
    if (packet.getPayloadType() == PayloadType::Ack) {
        uint32_t ack;
        std::memcpy(&ack, packet.payload, 4);
        return findAck(ack);
    }
    uint8_t hash[MAX_HASH_SIZE];
    calculateHash(packet, hash);
    return findHash(hash);
}
uint16_t PacketHistory::getHashCount() const {
    uint16_t count = 0;
    const uint8_t* sp = hashes_;
    const uint8_t zero[MAX_HASH_SIZE] = {0};
        for (int i = 0; i < MAX_PACKET_HASHES; i++, sp += MAX_HASH_SIZE) {
        if (std::memcmp(sp, zero, MAX_HASH_SIZE) != 0) {
            count++;
        }
    }
    return count;
}
uint8_t PacketHistory::getAckCount() const {
    uint8_t count = 0;
    for (int i = 0; i < MAX_PACKET_ACKS; i++) {
        if (acks_[i] != 0) {
            count++;
        }
    }
    return count;
}
void PacketHistory::clearAll() {
    std::memset(hashes_, 0, sizeof(hashes_));
    nextHashIdx_ = 0;
    std::memset(acks_, 0, sizeof(acks_));
    nextAckIdx_ = 0;
    directDups_ = floodDups_ = 0;
}
bool PacketHistory::findHash(const uint8_t* hash) const {

    const uint8_t* sp = hashes_;
    for (int i = 0; i < MAX_PACKET_HASHES; i++, sp += MAX_HASH_SIZE) {
        if (std::memcmp(hash, sp, MAX_HASH_SIZE) == 0) {
            return true;
        }
    }
    return false;
}
void PacketHistory::addHash(const uint8_t* hash) {

    std::memcpy(&hashes_[nextHashIdx_ * MAX_HASH_SIZE], hash, MAX_HASH_SIZE);
    nextHashIdx_ = (nextHashIdx_ + 1) % MAX_PACKET_HASHES;
}
void PacketHistory::clearHash(const uint8_t* hash) {
    uint8_t* sp = hashes_;
    for (int i = 0; i < MAX_PACKET_HASHES; i++, sp += MAX_HASH_SIZE) {
        if (std::memcmp(hash, sp, MAX_HASH_SIZE) == 0) {
            std::memset(sp, 0, MAX_HASH_SIZE);
            break;
        }
    }
}
bool PacketHistory::findAck(uint32_t ack) const {
    for (int i = 0; i < MAX_PACKET_ACKS; i++) {
        if (ack == acks_[i]) {
            return true;
        }
    }
    return false;
}
void PacketHistory::addAck(uint32_t ack) {

    acks_[nextAckIdx_] = ack;
    nextAckIdx_ = (nextAckIdx_ + 1) % MAX_PACKET_ACKS;
}
void PacketHistory::clearAck(uint32_t ack) {
    for (int i = 0; i < MAX_PACKET_ACKS; i++) {
        if (ack == acks_[i]) {
            acks_[i] = 0;
            break;
        }
    }
}
void PacketHistory::calculateHash(const Packet& packet, uint8_t* hash) {
    sha256_context ctx;
    sha256_init(&ctx);

    uint8_t payloadType = static_cast<uint8_t>(packet.getPayloadType());
    sha256_update(&ctx, &payloadType, 1);

    if (packet.getPayloadType() == PayloadType::Trace) {
        sha256_update(&ctx, &packet.pathLen, sizeof(packet.pathLen));
    }

    sha256_update(&ctx, packet.payload, packet.payloadLen);

    uint8_t fullHash[Config::PUB_KEY_SIZE];
    sha256_final(&ctx, fullHash);
    std::memcpy(hash, fullHash, MAX_HASH_SIZE);
}
}
