#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * NeighborTracker - Maintains a table of nearby repeaters
 * 
 * Tracks up to 8 neighbors based on received ADVERT packets.
 * Stores node hash (first byte of public key) and average SNR for link quality monitoring.
 */
class NeighborTracker {
public:
  static constexpr uint8_t MAX_NEIGHBORS = 8;
  
  struct Neighbor {
    uint8_t nodeHash;       // First byte of public key (node hash)
    int8_t avgSnr;          // Average SNR in dB
    uint8_t sampleCount;    // Number of samples for averaging
  };
  
  static NeighborTracker &getInstance();
  
  // Update neighbor table with new advert packet (replaces weakest when full)
  void updateNeighbor(uint8_t nodeHash, int8_t snr);
  
  // Get neighbor count (active neighbors)
  uint8_t getNeighborCount() const;
  
  // Get neighbor by index (for iteration)
  const Neighbor* getNeighbor(uint8_t index) const;
  
  // Build compact neighbor list string
  void buildNeighborList(char *dest, size_t maxLen) const;
  
  // Clear all neighbors
  void clear();

private:
  NeighborTracker() : neighborCount(0) {}
  
  Neighbor neighbors[MAX_NEIGHBORS];
  uint8_t neighborCount;
  
  // Find neighbor by hash (-1 if not found)
  int8_t findNeighbor(uint8_t nodeHash) const;
  
  // Find weakest neighbor (lowest SNR) for replacement
  int8_t findWeakestNeighbor() const;
  
  NeighborTracker(const NeighborTracker &) = delete;
  NeighborTracker &operator=(const NeighborTracker &) = delete;
};

