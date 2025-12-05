#include "CryptoIdentity.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>
#include "../../lib/ed25519/ed_25519.h"
#include "../crypto/CryptoUtils.h"
#include "../mesh/MeshCrypto.h"
#include "Logger.h"

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

// WARNING: Entropy collection uses weak sources (analog noise, timing jitter).
// Suitable for device identification but NOT cryptographically secure.
// For high-security apps, pre-generate keys on a PC and load onto device.
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

