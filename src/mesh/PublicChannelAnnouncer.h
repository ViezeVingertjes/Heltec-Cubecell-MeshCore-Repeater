#pragma once

#include <Arduino.h>
#include "../core/NodeConfig.h"
#include "../crypto/CryptoUtils.h"
#include "../mesh/MeshCrypto.h"
#include "../core/Config.h"
#include "../core/CryptoIdentity.h"
#include "../radio/LoRaTransmitter.h"
#include "../core/PacketDecoder.h"

class PublicChannelAnnouncer {
public:
  static PublicChannelAnnouncer &getInstance();

  void initialize();
  bool sendText(const char *text, uint32_t timestampSeconds);
  bool decodeMessage(const MeshCore::DecodedPacket &packet, uint32_t &timestamp,
                     char *textBuffer, size_t textBufferLen);
  bool buildPacket(const char *text, uint8_t *rawPacket, uint16_t &length,
                   uint32_t timestampSeconds);
  static constexpr size_t MAX_MESSAGE_LEN = 80;

private:
  PublicChannelAnnouncer();

  bool channelReady;
  uint8_t channelSecret[MeshCoreCompat::PUB_KEY_SIZE];
  uint8_t channelHash[MeshCoreCompat::PATH_HASH_SIZE];
};

