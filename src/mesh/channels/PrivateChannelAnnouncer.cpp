#include "PrivateChannelAnnouncer.h"

#include <string.h>
#include "../../core/Config.h"
#include "../../core/Logger.h"
#include "../../core/TimeSync.h"
#include "../../crypto/CryptoUtils.h"
#include "../../radio/LoRaTransmitter.h"

using namespace MeshCoreCompat;

PrivateChannelAnnouncer &PrivateChannelAnnouncer::getInstance() {
  static PrivateChannelAnnouncer instance;
  return instance;
}

PrivateChannelAnnouncer::PrivateChannelAnnouncer() : numChannels(0) {
  for (size_t i = 0; i < MAX_PRIVATE_CHANNELS; i++) {
    memset(channels[i].secret, 0, sizeof(channels[i].secret));
    memset(channels[i].hash, 0, sizeof(channels[i].hash));
    channels[i].ready = false;
  }
}

void PrivateChannelAnnouncer::initialize() {
  numChannels = Config::Channels::NUM_PRIVATE_CHANNELS;
  if (numChannels > MAX_PRIVATE_CHANNELS) {
    numChannels = MAX_PRIVATE_CHANNELS;
    LOG_WARN_FMT("Too many private channels configured, limiting to %d", MAX_PRIVATE_CHANNELS);
  }
  
  for (uint8_t ch = 0; ch < numChannels; ch++) {
    const char* hexKey = Config::Channels::PRIVATE_CHANNEL_KEYS[ch];
    
    // Decode hex string to bytes
    size_t hexLen = strlen(hexKey);
    if (hexLen != MeshCoreCompat::CIPHER_KEY_SIZE * 2) {
      LOG_ERROR_FMT("Invalid private channel %d PSK hex length", ch);
      channels[ch].ready = false;
      continue;
    }
    
    for (size_t i = 0; i < MeshCoreCompat::CIPHER_KEY_SIZE; i++) {
      char byte_str[3] = {hexKey[i*2], hexKey[i*2+1], '\0'};
      channels[ch].secret[i] = (uint8_t)strtol(byte_str, nullptr, 16);
    }

    MeshCrypto::CryptoUtils::sha256(channels[ch].hash, sizeof(channels[ch].hash),
                                    channels[ch].secret, MeshCoreCompat::CIPHER_KEY_SIZE);
    channels[ch].ready = true;
    LOG_INFO_FMT("Initialized private channel %d", ch);
  }
}

bool PrivateChannelAnnouncer::buildPacket(const char *text, uint8_t *rawPacket,
                                         uint16_t &length,
                                         uint32_t timestampSeconds,
                                         uint8_t channelIndex) {
  if (channelIndex >= numChannels || !channels[channelIndex].ready) {
    return false;
  }
  return buildPacketInternal(text, rawPacket, length, timestampSeconds,
                             channels[channelIndex].secret, channels[channelIndex].hash);
}

bool PrivateChannelAnnouncer::sendText(const char *text,
                                      uint32_t timestampSeconds,
                                      uint8_t channelIndex) {
  if (channelIndex >= numChannels || !channels[channelIndex].ready) {
    return false;
  }

  uint8_t raw[Config::Forwarding::MAX_ENCODED_PACKET_SIZE];
  uint16_t len = 0;
  if (!buildPacket(text, raw, len, timestampSeconds, channelIndex)) {
    return false;
  }

  auto &tx = LoRaTransmitter::getInstance();
  return tx.transmit(raw, len);
}

bool PrivateChannelAnnouncer::decodeMessage(
    const MeshCore::DecodedPacket &packet, uint32_t &timestamp, char *textBuffer,
    size_t textBufferLen, uint8_t &channelIndex) {
  if (packet.payloadType != MeshCore::PayloadType::GRP_TXT) {
    return false;
  }

  if (packet.payloadLength <= MeshCoreCompat::PATH_HASH_SIZE +
                                 MeshCoreCompat::CIPHER_MAC_SIZE) {
    return false;
  }

  // Try all configured channels
  for (uint8_t ch = 0; ch < numChannels; ch++) {
    if (!channels[ch].ready) {
      continue;
    }

    // Try to decode with this channel
    if (decodeMessageInternal(packet, timestamp, textBuffer, textBufferLen,
                              channels[ch].secret, channels[ch].hash)) {
      channelIndex = ch;
      return true;
    }
  }

  return false;  // No channel matched
}

