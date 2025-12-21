#pragma once

#include <Arduino.h>

namespace MeshCore {

class NodeConfig {
public:
  static NodeConfig &getInstance();

  void initialize();
  uint16_t getNodeId() const { return nodeId; }
  uint8_t getNodeHash() const { return nodeHash; }
  
  // Location management
  bool hasLocation() const { return locationSet; }
  int32_t getLatitude() const { return latitude; }
  int32_t getLongitude() const { return longitude; }
  void setLocation(int32_t lat, int32_t lon);
  void clearLocation();

private:
  NodeConfig() : nodeId(0), nodeHash(0), locationSet(false), latitude(0), longitude(0) {}

  uint16_t nodeId;
  uint8_t nodeHash;
  bool locationSet;
  int32_t latitude;   // in microdegrees
  int32_t longitude;  // in microdegrees

  uint16_t generateNodeIdFromChip();
  uint8_t generateNodeHashFromChip();
  void loadLocationFromEEPROM();
  void saveLocationToEEPROM();

  NodeConfig(const NodeConfig &) = delete;
  NodeConfig &operator=(const NodeConfig &) = delete;
};

} // namespace MeshCore
