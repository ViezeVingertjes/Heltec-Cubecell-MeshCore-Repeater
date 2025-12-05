#pragma once

#include <Arduino.h>

namespace MeshCrypto {

/**
 * Minimal AES-128 ECB implementation (encrypt/decrypt single blocks).
 * Based on the public domain tiny-AES-c reference.
 */
class AES128 {
public:
  AES128();

  void setKey(const uint8_t key[16]);
  void encryptBlock(uint8_t output[16], const uint8_t input[16]) const;
  void decryptBlock(uint8_t output[16], const uint8_t input[16]) const;

private:
  uint8_t roundKey[176];

  void keyExpansion(const uint8_t key[16]);
};

} // namespace MeshCrypto

