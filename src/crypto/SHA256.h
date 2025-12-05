#pragma once

#include <Arduino.h>

namespace MeshCrypto {

struct SHA256Context {
  uint32_t state[8];
  uint64_t bitCount;
  uint8_t buffer[64];
};

class SHA256 {
public:
  SHA256();

  void reset();
  void update(const uint8_t *data, size_t len);
  void finalize(uint8_t digest[32]);

  static void hmac(const uint8_t *key, size_t keyLen, const uint8_t *data,
                   size_t dataLen, uint8_t digest[32]);

private:
  void transform(const uint8_t block[64]);

  SHA256Context ctx;
};

} // namespace MeshCrypto

