#pragma once

#include <stddef.h>
#include <stdint.h>
#include "../../core/PacketDecoder.h"

// Base class with shared channel functionality
class ChannelAnnouncer {
public:
  static constexpr size_t MAX_MESSAGE_LEN = 160;  // 10*CIPHER_BLOCK_SIZE, matches MeshCore

protected:
  // Shared packet building logic
  static bool buildPacketInternal(const char *text, uint8_t *rawPacket, uint16_t &length,
                                  uint32_t timestampSeconds,
                                  const uint8_t *channelSecret, const uint8_t *channelHash);
  
  // Shared decoding logic  
  static bool decodeMessageInternal(const MeshCore::DecodedPacket &packet, 
                                    uint32_t &timestamp, char *textBuffer,
                                    size_t textBufferLen,
                                    const uint8_t *channelSecret, const uint8_t *channelHash);
};

