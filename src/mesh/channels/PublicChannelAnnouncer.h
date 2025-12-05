#pragma once

#include <stdint.h>
#include "../MeshCrypto.h"
#include "../../core/PacketDecoder.h"
#include "ChannelAnnouncer.h"

class PublicChannelAnnouncer : public ChannelAnnouncer {
public:
  static PublicChannelAnnouncer &getInstance();

  void initialize();
  bool sendText(const char *text, uint32_t timestampSeconds);
  bool decodeMessage(const MeshCore::DecodedPacket &packet, uint32_t &timestamp,
                     char *textBuffer, size_t textBufferLen);
  bool buildPacket(const char *text, uint8_t *rawPacket, uint16_t &length,
                   uint32_t timestampSeconds);

private:
  PublicChannelAnnouncer();

  bool channelReady;
  uint8_t channelSecret[MeshCoreCompat::PUB_KEY_SIZE];
  uint8_t channelHash[MeshCoreCompat::PATH_HASH_SIZE];
};

