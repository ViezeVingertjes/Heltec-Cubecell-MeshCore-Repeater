#pragma once

#include <stdint.h>
#include "../MeshCrypto.h"
#include "../../core/PacketDecoder.h"
#include "ChannelAnnouncer.h"

class PrivateChannelAnnouncer : public ChannelAnnouncer {
public:
  static PrivateChannelAnnouncer &getInstance();

  void initialize();
  bool sendText(const char *text, uint32_t timestampSeconds, uint8_t channelIndex = 0);
  bool decodeMessage(const MeshCore::DecodedPacket &packet, uint32_t &timestamp,
                     char *textBuffer, size_t textBufferLen, uint8_t &channelIndex);
  bool buildPacket(const char *text, uint8_t *rawPacket, uint16_t &length,
                   uint32_t timestampSeconds, uint8_t channelIndex = 0);
  uint8_t getNumChannels() const { return numChannels; }
  static constexpr size_t MAX_PRIVATE_CHANNELS = 8;

private:
  PrivateChannelAnnouncer();

  struct ChannelData {
    uint8_t secret[MeshCoreCompat::PUB_KEY_SIZE];
    uint8_t hash[MeshCoreCompat::PATH_HASH_SIZE];
    bool ready;
  };

  ChannelData channels[MAX_PRIVATE_CHANNELS];
  uint8_t numChannels;
};

