#pragma once

#include <stdint.h>

namespace HashUtils {

// FNV-1a hash constants
constexpr uint32_t FNV_OFFSET_BASIS = 2166136261u;
constexpr uint32_t FNV_PRIME = 16777619u;

// Compute FNV-1a hash of a byte array
inline uint32_t fnv1a(const uint8_t* data, uint16_t length) {
  uint32_t hash = FNV_OFFSET_BASIS;
  for (uint16_t i = 0; i < length; ++i) {
    hash ^= data[i];
    hash *= FNV_PRIME;
  }
  return hash;
}

// Compute FNV-1a hash with additional byte values
inline uint32_t fnv1aWithBytes(const uint8_t* data, uint16_t length, uint8_t byte1, uint8_t byte2) {
  uint32_t hash = FNV_OFFSET_BASIS;
  hash ^= byte1;
  hash *= FNV_PRIME;
  hash ^= byte2;
  hash *= FNV_PRIME;
  for (uint16_t i = 0; i < length; ++i) {
    hash ^= data[i];
    hash *= FNV_PRIME;
  }
  return hash;
}

} // namespace HashUtils

