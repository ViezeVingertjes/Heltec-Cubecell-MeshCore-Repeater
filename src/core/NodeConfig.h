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
  NodeConfig() : nodeId(0), nodeHash(0) {}

  uint16_t nodeId;
  uint8_t nodeHash;

  uint16_t generateNodeIdFromChip();
  uint8_t generateNodeHashFromChip();

  NodeConfig(const NodeConfig &) = delete;
  NodeConfig &operator=(const NodeConfig &) = delete;
};

} // namespace MeshCore
