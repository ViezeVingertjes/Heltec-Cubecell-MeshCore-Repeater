#pragma once

#include <Arduino.h>
#include "../mesh/MeshCrypto.h"

class CryptoIdentity {
public:
  static CryptoIdentity &getInstance();

  void initialize();
  const uint8_t *getPublicKey() const { return publicKey; }
  const uint8_t *getPrivateKey() const { return privateKey; }

private:
  CryptoIdentity();

  static constexpr uint8_t STORAGE_MAGIC = 0xC5;
  static constexpr size_t STORAGE_SIZE =
      1 + MeshCoreCompat::PUB_KEY_SIZE + MeshCoreCompat::PRV_KEY_SIZE;

  uint8_t publicKey[MeshCoreCompat::PUB_KEY_SIZE];
  uint8_t privateKey[MeshCoreCompat::PRV_KEY_SIZE];

  bool loadFromEEPROM();
  void saveToEEPROM();
  void generateNewIdentity();
  void collectEntropy(uint8_t *dest, size_t len);
};

