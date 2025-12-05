#include "ChannelAnnouncer.h"

#include <string.h>
#include "../../core/Config.h"
#include "../../core/TimeSync.h"
#include "../../crypto/CryptoUtils.h"
#include "../MeshCrypto.h"

using namespace MeshCoreCompat;

bool ChannelAnnouncer::buildPacketInternal(const char *text, uint8_t *rawPacket,
                                           uint16_t &length, uint32_t timestampSeconds,
                                           const uint8_t *channelSecret, 
                                           const uint8_t *channelHash) {
  MeshCore::DecodedPacket packet{};
  packet.routeType = MeshCore::RouteType::FLOOD;
  packet.payloadType = MeshCore::PayloadType::GRP_TXT;
  packet.payloadVersion = 0;
  packet.hasTransportCodes = false;
  packet.pathLength = 0;

  packet.header = static_cast<uint8_t>(MeshCore::RouteType::FLOOD) |
                  (static_cast<uint8_t>(MeshCore::PayloadType::GRP_TXT)
                   << MeshCore::PH_TYPE_SHIFT);

  uint8_t plaintext[5 + MAX_MESSAGE_LEN];
  memcpy(&plaintext[0], &timestampSeconds, sizeof(timestampSeconds));
  plaintext[4] = 0; // attempt/flags

  size_t textLen = 0;
  while (text[textLen] != '\0' && textLen < MAX_MESSAGE_LEN - 1) {
    ++textLen;
  }
  memcpy(&plaintext[5], text, textLen);
  plaintext[5 + textLen] = '\0';
  int plainLen = 5 + static_cast<int>(textLen);

  uint8_t cipher[CIPHER_MAC_SIZE + MAX_MESSAGE_LEN + 16];
  int encLen = MeshCrypto::CryptoUtils::encryptThenMAC(channelSecret, cipher, 
                                                       plaintext, plainLen);
  if (encLen <= 0 || encLen + PATH_HASH_SIZE > MeshCore::MAX_PACKET_PAYLOAD) {
    return false;
  }

  memcpy(packet.payload, channelHash, PATH_HASH_SIZE);
  memcpy(packet.payload + PATH_HASH_SIZE, cipher, encLen);
  packet.payloadLength = PATH_HASH_SIZE + encLen;

  length = MeshCore::PacketDecoder::encode(packet, rawPacket, 
                                           Config::Forwarding::MAX_ENCODED_PACKET_SIZE);
  return length > 0;
}

bool ChannelAnnouncer::decodeMessageInternal(const MeshCore::DecodedPacket &packet,
                                             uint32_t &timestamp, char *textBuffer,
                                             size_t textBufferLen,
                                             const uint8_t *channelSecret,
                                             const uint8_t *channelHash) {
  if (packet.payloadType != MeshCore::PayloadType::GRP_TXT) {
    return false;
  }

  if (packet.payloadLength <= PATH_HASH_SIZE + CIPHER_MAC_SIZE) {
    return false;
  }

  if (memcmp(packet.payload, channelHash, PATH_HASH_SIZE) != 0) {
    return false;
  }

  int cipherLen = packet.payloadLength - PATH_HASH_SIZE;
  uint8_t plaintext[5 + MAX_MESSAGE_LEN];
  int decLen = MeshCrypto::CryptoUtils::MACThenDecrypt(channelSecret, plaintext,
                                                       packet.payload + PATH_HASH_SIZE,
                                                       cipherLen);
  if (decLen < 5) {
    return false;
  }

  memcpy(&timestamp, plaintext, sizeof(timestamp));
  TimeSync::updateFromRemote(timestamp);

  size_t clampIndex = decLen < static_cast<int>(sizeof(plaintext))
                          ? static_cast<size_t>(decLen)
                          : sizeof(plaintext) - 1;
  plaintext[clampIndex] = '\0';
  const char *textPtr = reinterpret_cast<const char *>(&plaintext[5]);
  size_t maxCopy = textBufferLen > 0 ? textBufferLen - 1 : 0;
  size_t actualLen = strnlen(textPtr, MAX_MESSAGE_LEN - 1);
  if (actualLen > maxCopy) {
    actualLen = maxCopy;
  }
  if (textBufferLen > 0) {
    memcpy(textBuffer, textPtr, actualLen);
    textBuffer[actualLen] = '\0';
  }
  return true;
}

