#pragma once
#include <cstdint>
#include "core/config.h"
#include "packet/packet.h"
#include "packet/packet_history.h"
#include "core/result.h"
namespace MiniCore {
using Config::DEFAULT_MAX_FLOOD_PATH;
using Config::MAX_HASH_SIZE;
enum class RepeaterAction : uint8_t {
    Drop = 0,
    Forward = 1
};
struct RepeaterConfig {
    bool enabled = true;
    uint8_t maxFloodPath = DEFAULT_MAX_FLOOD_PATH;
    bool dropRepeaterAdverts = false;
};
class Repeater {
public:
    Repeater(PacketHistory& history, const RepeaterConfig& config);
    [[nodiscard]] PacketHistory& history() { return history_; }
    void setSelfHash(uint8_t hash);
    void setSelfHashMultiByte(const uint8_t* hash, uint8_t len);
    RepeaterAction shouldForward(const Packet& packet);
    [[nodiscard]] uint32_t calculateRetransmitDelay(const Packet& packet, uint32_t baseAirtime) const;
    [[nodiscard]] uint8_t getPriority(const Packet& packet) const;
        Status appendSelfHash(Packet& packet, uint8_t selfHash);
    void removeSelfFromPath(Packet& packet);
    void appendSnrToPath(Packet& packet);
private:
    RepeaterAction shouldForwardTrace(const Packet& packet);
    [[nodiscard]] bool isHashMatch(const uint8_t* hash, uint8_t len) const;
    [[nodiscard]] bool isFloodPacketDestinedForSelf(const Packet& packet) const;
    [[nodiscard]] bool isRepeaterOrRoomAdvert(const Packet& packet) const;
    bool hasSeenPacket(const Packet& packet);
        PacketHistory& history_;
    RepeaterConfig config_;
    uint8_t selfHash_[MAX_HASH_SIZE]{};
};
}
