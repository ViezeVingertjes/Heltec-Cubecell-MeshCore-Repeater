#include "NeighborTracker.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include "../core/Logger.h"

NeighborTracker &NeighborTracker::getInstance() {
  static NeighborTracker instance;
  return instance;
}

void NeighborTracker::updateNeighbor(uint8_t nodeHash, int8_t snr) {
  if (nodeHash == 0 || nodeHash == 0xFF) {
    return; // Invalid node hash
  }
  
  int8_t idx = findNeighbor(nodeHash);
  
  if (idx >= 0) {
    // Update existing neighbor with exponential moving average
    // EMA: new_avg = (alpha * new_value) + ((1-alpha) * old_avg)
    // Using alpha = 0.25 (weighted toward history)
    Neighbor &n = neighbors[idx];
    n.avgSnr = (snr + (n.avgSnr * 3)) / 4;
    if (n.sampleCount < 255) n.sampleCount++;
    
    LOG_DEBUG_FMT("Updated neighbor %02X: SNR %d dB (avg %d dB)", 
                  nodeHash, snr, n.avgSnr);
  } else if (neighborCount < MAX_NEIGHBORS) {
    // Add new neighbor
    neighbors[neighborCount].nodeHash = nodeHash;
    neighbors[neighborCount].avgSnr = snr;
    neighbors[neighborCount].sampleCount = 1;
    neighborCount++;
    
    LOG_INFO_FMT("New neighbor %02X: SNR %d dB", nodeHash, snr);
  } else {
    // Table full - replace weakest neighbor if new one is stronger
    int8_t weakestIdx = findWeakestNeighbor();
    if (weakestIdx >= 0 && snr > neighbors[weakestIdx].avgSnr) {
      LOG_INFO_FMT("Replacing weak neighbor %02X (SNR %d) with %02X (SNR %d)", 
                   neighbors[weakestIdx].nodeHash, neighbors[weakestIdx].avgSnr,
                   nodeHash, snr);
      
      neighbors[weakestIdx].nodeHash = nodeHash;
      neighbors[weakestIdx].avgSnr = snr;
      neighbors[weakestIdx].sampleCount = 1;
    } else {
      LOG_DEBUG_FMT("Ignoring neighbor %02X (SNR %d), table full with stronger links", 
                    nodeHash, snr);
    }
  }
}

uint8_t NeighborTracker::getNeighborCount() const {
  return neighborCount;
}

const NeighborTracker::Neighbor* NeighborTracker::getNeighbor(uint8_t index) const {
  if (index >= neighborCount) return nullptr;
  return &neighbors[index];
}

void NeighborTracker::buildNeighborList(char *dest, size_t maxLen) const {
  if (neighborCount == 0) {
    snprintf(dest, maxLen, "No neighbors");
    return;
  }
  
  // Create sorted indices (by SNR, highest first)
  uint8_t sorted[MAX_NEIGHBORS];
  for (uint8_t i = 0; i < neighborCount; i++) {
    sorted[i] = i;
  }
  
  // Simple selection sort by SNR (descending)
  for (uint8_t i = 0; i < neighborCount - 1; i++) {
    for (uint8_t j = i + 1; j < neighborCount; j++) {
      if (neighbors[sorted[j]].avgSnr > neighbors[sorted[i]].avgSnr) {
        uint8_t temp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = temp;
      }
    }
  }
  
  // Build compact list: "7316:12 5A42:8 ..." (ID:SNR pairs, sorted by SNR)
  size_t offset = 0;
  
  for (uint8_t i = 0; i < neighborCount && offset < maxLen - 10; i++) {
    const Neighbor &n = neighbors[sorted[i]];
    
    if (i > 0) {
      dest[offset++] = ' ';
    }
    
    // Format: XX:YY where XX is hex hash (2 digits), YY is SNR
    int written = snprintf(&dest[offset], maxLen - offset, "%02X:%d", 
                          n.nodeHash, n.avgSnr);
    if (written > 0) {
      offset += written;
    }
  }
  dest[offset] = '\0';
}

void NeighborTracker::clear() {
  neighborCount = 0;
  LOG_INFO("Neighbor table cleared");
}

int8_t NeighborTracker::findNeighbor(uint8_t nodeHash) const {
  for (uint8_t i = 0; i < neighborCount; i++) {
    if (neighbors[i].nodeHash == nodeHash) {
      return i;
    }
  }
  return -1;
}

int8_t NeighborTracker::findWeakestNeighbor() const {
  if (neighborCount == 0) return -1;
  
  int8_t weakestIdx = 0;
  int8_t weakestSnr = neighbors[0].avgSnr;
  
  for (uint8_t i = 1; i < neighborCount; i++) {
    if (neighbors[i].avgSnr < weakestSnr) {
      weakestSnr = neighbors[i].avgSnr;
      weakestIdx = i;
    }
  }
  
  return weakestIdx;
}

