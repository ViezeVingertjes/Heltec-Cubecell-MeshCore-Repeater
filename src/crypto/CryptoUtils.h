#pragma once

#include <Arduino.h>
#include "../mesh/MeshCrypto.h"

namespace MeshCrypto {

class CryptoUtils {
public:
  static void sha256(uint8_t *hash, size_t hashLen, const uint8_t *msg,
                     size_t msgLen);
  static int encrypt(const uint8_t *sharedSecret, uint8_t *dest,
                     const uint8_t *src, int srcLen);
  static int decrypt(const uint8_t *sharedSecret, uint8_t *dest,
                     const uint8_t *src, int srcLen);
  static int encryptThenMAC(const uint8_t *sharedSecret, uint8_t *dest,
                            const uint8_t *src, int srcLen);
  static int MACThenDecrypt(const uint8_t *sharedSecret, uint8_t *dest,
                            const uint8_t *src, int srcLen);
};

int base64Decode(const char *input, uint8_t *output, size_t maxLen);

} // namespace MeshCrypto

