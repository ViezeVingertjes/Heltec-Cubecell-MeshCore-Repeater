#include "CryptoIdentity.h"
#include <EEPROM.h>
#include "Logger.h"
#include "../mesh/MeshCrypto.h"
#include "../crypto/CryptoUtils.h"
#include "../../lib/ed25519/ed_25519.h"

CryptoIdentity &CryptoIdentity::getInstance() {
  static CryptoIdentity instance;
  return instance;
}

CryptoIdentity::CryptoIdentity() {
  memset(publicKey, 0, sizeof(publicKey));
  memset(privateKey, 0, sizeof(privateKey));
}

void CryptoIdentity::initialize() {
  EEPROM.begin(STORAGE_SIZE);
  if (loadFromEEPROM()) {
    LOG_INFO("Loaded persisted identity");
    return;
  }

  generateNewIdentity();
  saveToEEPROM();
  LOG_INFO("Generated new identity");
}

bool CryptoIdentity::loadFromEEPROM() {
  if (EEPROM.read(0) != STORAGE_MAGIC) {
    return false;
  }

  for (size_t i = 0; i < MeshCoreCompat::PUB_KEY_SIZE; ++i) {
    publicKey[i] = EEPROM.read(1 + i);
  }
  for (size_t i = 0; i < MeshCoreCompat::PRV_KEY_SIZE; ++i) {
    privateKey[i] = EEPROM.read(1 + MeshCoreCompat::PUB_KEY_SIZE + i);
  }
  return true;
}

void CryptoIdentity::saveToEEPROM() {
  EEPROM.write(0, STORAGE_MAGIC);
  for (size_t i = 0; i < MeshCoreCompat::PUB_KEY_SIZE; ++i) {
    EEPROM.write(1 + i, publicKey[i]);
  }
  for (size_t i = 0; i < MeshCoreCompat::PRV_KEY_SIZE; ++i) {
    EEPROM.write(1 + MeshCoreCompat::PUB_KEY_SIZE + i, privateKey[i]);
  }
  EEPROM.commit();
}

// WARNING: This entropy collection method uses weak sources and is NOT
// cryptographically secure. It relies on analog noise, timing jitter, and
// Arduino's pseudo-random number generator. For production deployments requiring
// high security, consider:
// 1. Using a hardware RNG if available
// 2. Pre-generating keys in a high-entropy environment
// 3. Loading externally generated keys
// The current implementation is suitable for device identification but may not
// provide sufficient entropy for high-security cryptographic applications.
void CryptoIdentity::collectEntropy(uint8_t *dest, size_t len) {
  randomSeed(analogRead(ADC) ^ micros());
  for (size_t i = 0; i < len; ++i) {
    uint32_t sample = micros() ^ millis();
    sample ^= analogRead(ADC);
    sample ^= random(0xFFFFFFFF);
    dest[i] = sample & 0xFF;
    delay(2);
  }
}

void CryptoIdentity::generateNewIdentity() {
  uint8_t seed[MeshCoreCompat::SEED_SIZE];
  collectEntropy(seed, sizeof(seed));
  ed25519_create_keypair(publicKey, privateKey, seed);
  memset(seed, 0, sizeof(seed));
}

