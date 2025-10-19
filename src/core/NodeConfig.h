#pragma once

#include <Arduino.h>

namespace MeshCore {

class NodeConfig {
public:
  static NodeConfig &getInstance();

  void initialize();
  uint16_t getNodeId() const { return nodeId; }
  uint8_t getNodeHash() const { return nodeHash; }

private:
  NodeConfig() : nodeId(0) {}

  static constexpr uint32_t EEPROM_MAGIC = 0x4D534843; // "MSHC"
  static constexpr uint16_t EEPROM_ADDR = 0;

  struct StoredConfig {
    uint32_t magic;
    uint16_t nodeId;
    uint16_t checksum;
  };

  uint16_t nodeId;
  uint8_t nodeHash;

  uint16_t generateNodeIdFromChip();
  uint8_t generateNodeHashFromChip();
  uint16_t calculateChecksum(const StoredConfig &config);
  bool loadFromEEPROM();
  void saveToEEPROM();

  NodeConfig(const NodeConfig &) = delete;
  NodeConfig &operator=(const NodeConfig &) = delete;
};

} // namespace MeshCore
