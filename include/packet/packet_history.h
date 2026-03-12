#pragma once
#include <cstdint>
#include <cstring>
#include "core/config.h"
#include "packet/packet.h"
namespace MiniCore {
using Config::MAX_HASH_SIZE;
using Config::MAX_PACKET_HASHES;
using Config::MAX_PACKET_ACKS;
class PacketHistory {
public:
    PacketHistory();
    bool hasSeen(const Packet& packet);
    void clear(const Packet& packet);
    [[nodiscard]] bool contains(const Packet& packet) const;
    [[nodiscard]] uint16_t getHashCount() const;
    [[nodiscard]] uint8_t getAckCount() const;
    void clearAll();
    [[nodiscard]] uint32_t getDirectDups() const { return directDups_; }
    [[nodiscard]] uint32_t getFloodDups() const { return floodDups_; }
    void resetStats() { directDups_ = floodDups_ = 0; }
    static void calculateHash(const Packet& packet, uint8_t* hash);
private:

    uint8_t hashes_[MAX_PACKET_HASHES * MAX_HASH_SIZE];
    int nextHashIdx_;

    uint32_t acks_[MAX_PACKET_ACKS];
    int nextAckIdx_;

    uint32_t directDups_;
    uint32_t floodDups_;

    bool findHash(const uint8_t* hash) const;
    void addHash(const uint8_t* hash);
    void clearHash(const uint8_t* hash);
    bool findAck(uint32_t ack) const;
    void addAck(uint32_t ack);
    void clearAck(uint32_t ack);
};
}
