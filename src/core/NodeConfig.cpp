#include "NodeConfig.h"
#include "Config.h"
#include "CyLib.h"
#include "Logger.h"

namespace MeshCore {

NodeConfig &NodeConfig::getInstance() {
  static NodeConfig instance;
  return instance;
}

void NodeConfig::initialize() {
  // Check if custom node ID is enabled
  if (Config::Identity::USE_CUSTOM_NODE_ID) {
    nodeId = Config::Identity::CUSTOM_NODE_ID;
    LOG_INFO_FMT("Node ID: 0x%04X (custom)", nodeId);
  } else {
    nodeId = generateNodeIdFromChip();
    LOG_INFO_FMT("Node ID: 0x%04X (from chip)", nodeId);
  }
  
  // Check if custom node hash is enabled
  if (Config::Identity::USE_CUSTOM_NODE_HASH) {
    nodeHash = Config::Identity::CUSTOM_NODE_HASH;
    LOG_INFO_FMT("Node Hash: 0x%02X (custom)", nodeHash);
  } else {
    nodeHash = generateNodeHashFromChip();
    LOG_INFO_FMT("Node Hash: 0x%02X (from chip)", nodeHash);
  }
}

uint16_t NodeConfig::generateNodeIdFromChip() {
  uint8_t dieX = CY_GET_XTND_REG8(reinterpret_cast<void *>(CYREG_SFLASH_DIE_X));
  uint8_t dieY = CY_GET_XTND_REG8(reinterpret_cast<void *>(CYREG_SFLASH_DIE_Y));
  uint8_t dieWafer =
      CY_GET_XTND_REG8(reinterpret_cast<void *>(CYREG_SFLASH_DIE_WAFER));
  uint8_t dieLot0 =
      CY_GET_XTND_REG8(reinterpret_cast<void *>(CYREG_SFLASH_DIE_LOT0));

  uint16_t id = ((uint16_t)dieX << 8) | dieY;
  id ^= ((uint16_t)dieWafer << 8) | dieLot0;

  if (id == 0 || id == 0xFFFF || id < 0x0100) {
    id = 0x7C00 | (id & 0x00FF);
  }

  return id;
}

uint8_t NodeConfig::generateNodeHashFromChip() {
  uint8_t dieX = CY_GET_XTND_REG8(reinterpret_cast<void *>(CYREG_SFLASH_DIE_X));
  uint8_t dieY = CY_GET_XTND_REG8(reinterpret_cast<void *>(CYREG_SFLASH_DIE_Y));
  uint8_t dieWafer =
      CY_GET_XTND_REG8(reinterpret_cast<void *>(CYREG_SFLASH_DIE_WAFER));
  uint8_t dieLot0 =
      CY_GET_XTND_REG8(reinterpret_cast<void *>(CYREG_SFLASH_DIE_LOT0));

  uint8_t hash = dieX ^ dieY ^ dieWafer ^ dieLot0;

  if (hash == 0 || hash == 0xFF) {
    hash = 0x7C;
  }

  return hash;
}

} // namespace MeshCore
