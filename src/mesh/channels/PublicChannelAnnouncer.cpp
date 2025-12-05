#include "PublicChannelAnnouncer.h"

#include <string.h>
#include "../../core/Config.h"
#include "../../core/Logger.h"
#include "../../core/TimeSync.h"
#include "../../crypto/CryptoUtils.h"
#include "../../radio/LoRaTransmitter.h"

using namespace MeshCoreCompat;

namespace {
constexpr char kPublicChannelKey[] = "izOH6cXN6mrJ5e26oRXNcg==";
}

PublicChannelAnnouncer &PublicChannelAnnouncer::getInstance() {
  static PublicChannelAnnouncer instance;
  return instance;
}

PublicChannelAnnouncer::PublicChannelAnnouncer() : channelReady(false) {
  memset(channelSecret, 0, sizeof(channelSecret));
  memset(channelHash, 0, sizeof(channelHash));
}

void PublicChannelAnnouncer::initialize() {
  memset(channelSecret, 0, sizeof(channelSecret));
  int len = MeshCrypto::base64Decode(kPublicChannelKey, channelSecret,
                                     sizeof(channelSecret));
  if (len != MeshCoreCompat::CIPHER_KEY_SIZE) {
    LOG_ERROR("Invalid public channel PSK length");
    channelReady = false;
    return;
  }

  MeshCrypto::CryptoUtils::sha256(channelHash, sizeof(channelHash),
                                  channelSecret, len);
  channelReady = true;
}

bool PublicChannelAnnouncer::buildPacket(const char *text, uint8_t *rawPacket,
                                         uint16_t &length,
                                         uint32_t timestampSeconds) {
  if (!channelReady) {
    return false;
  }
  return buildPacketInternal(text, rawPacket, length, timestampSeconds, 
                             channelSecret, channelHash);
}

bool PublicChannelAnnouncer::sendText(const char *text,
                                      uint32_t timestampSeconds) {
  if (!channelReady) {
    return false;
  }

  uint8_t raw[Config::Forwarding::MAX_ENCODED_PACKET_SIZE];
  uint16_t len = 0;
  if (!buildPacket(text, raw, len, timestampSeconds)) {
    return false;
  }

  auto &tx = LoRaTransmitter::getInstance();
  return tx.transmit(raw, len);
}

bool PublicChannelAnnouncer::decodeMessage(
    const MeshCore::DecodedPacket &packet, uint32_t &timestamp, char *textBuffer,
    size_t textBufferLen) {
  if (!channelReady) {
    return false;
  }
  return decodeMessageInternal(packet, timestamp, textBuffer, textBufferLen,
                               channelSecret, channelHash);
}

