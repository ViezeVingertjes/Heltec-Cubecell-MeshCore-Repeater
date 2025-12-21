#include "NodeConfig.h"
#include "Config.h"
#include "CyLib.h"
#include "Logger.h"
#include "CryptoIdentity.h"
#include <EEPROM.h>

namespace MeshCore {

// EEPROM addresses for persistent storage
static constexpr uint16_t EEPROM_LOCATION_MAGIC_ADDR = 0;
static constexpr uint16_t EEPROM_LOCATION_LAT_ADDR = 2;
static constexpr uint16_t EEPROM_LOCATION_LON_ADDR = 6;
static constexpr uint16_t LOCATION_MAGIC = 0x4C4F;  // "LO"

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
    // IMPORTANT: Node hash MUST be the first byte of the Ed25519 public key
    // This ensures consistency between advert packets and forwarding paths
    nodeHash = CryptoIdentity::getInstance().getPublicKey()[0];
    LOG_INFO_FMT("Node Hash: 0x%02X (from Ed25519 public key)", nodeHash);
  }
  
  // Load location from EEPROM (purely dynamic, no config fallback)
  loadLocationFromEEPROM();
  if (locationSet) {
    LOG_INFO_FMT("Location: %ld,%ld (from EEPROM)", latitude, longitude);
  } else {
    LOG_INFO("No location set (use !location lat lon)");
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

void NodeConfig::setLocation(int32_t lat, int32_t lon) {
  latitude = lat;
  longitude = lon;
  locationSet = true;
  saveLocationToEEPROM();
  LOG_INFO_FMT("Location set: %ld,%ld", latitude, longitude);
}

void NodeConfig::clearLocation() {
  locationSet = false;
  latitude = 0;
  longitude = 0;
  // Clear EEPROM magic to indicate no location
  EEPROM.write(EEPROM_LOCATION_MAGIC_ADDR, 0);
  EEPROM.write(EEPROM_LOCATION_MAGIC_ADDR + 1, 0);
  EEPROM.commit();
  LOG_INFO("Location cleared");
}

void NodeConfig::loadLocationFromEEPROM() {
  uint16_t magic = (EEPROM.read(EEPROM_LOCATION_MAGIC_ADDR) << 8) | 
                    EEPROM.read(EEPROM_LOCATION_MAGIC_ADDR + 1);
  
  if (magic == LOCATION_MAGIC) {
    // Read latitude (4 bytes)
    latitude = ((int32_t)EEPROM.read(EEPROM_LOCATION_LAT_ADDR) << 24) |
               ((int32_t)EEPROM.read(EEPROM_LOCATION_LAT_ADDR + 1) << 16) |
               ((int32_t)EEPROM.read(EEPROM_LOCATION_LAT_ADDR + 2) << 8) |
               ((int32_t)EEPROM.read(EEPROM_LOCATION_LAT_ADDR + 3));
    
    // Read longitude (4 bytes)
    longitude = ((int32_t)EEPROM.read(EEPROM_LOCATION_LON_ADDR) << 24) |
                ((int32_t)EEPROM.read(EEPROM_LOCATION_LON_ADDR + 1) << 16) |
                ((int32_t)EEPROM.read(EEPROM_LOCATION_LON_ADDR + 2) << 8) |
                ((int32_t)EEPROM.read(EEPROM_LOCATION_LON_ADDR + 3));
    
    locationSet = true;
  } else {
    locationSet = false;
  }
}

void NodeConfig::saveLocationToEEPROM() {
  // Write magic
  EEPROM.write(EEPROM_LOCATION_MAGIC_ADDR, (LOCATION_MAGIC >> 8) & 0xFF);
  EEPROM.write(EEPROM_LOCATION_MAGIC_ADDR + 1, LOCATION_MAGIC & 0xFF);
  
  // Write latitude (4 bytes)
  EEPROM.write(EEPROM_LOCATION_LAT_ADDR, (latitude >> 24) & 0xFF);
  EEPROM.write(EEPROM_LOCATION_LAT_ADDR + 1, (latitude >> 16) & 0xFF);
  EEPROM.write(EEPROM_LOCATION_LAT_ADDR + 2, (latitude >> 8) & 0xFF);
  EEPROM.write(EEPROM_LOCATION_LAT_ADDR + 3, latitude & 0xFF);
  
  // Write longitude (4 bytes)
  EEPROM.write(EEPROM_LOCATION_LON_ADDR, (longitude >> 24) & 0xFF);
  EEPROM.write(EEPROM_LOCATION_LON_ADDR + 1, (longitude >> 16) & 0xFF);
  EEPROM.write(EEPROM_LOCATION_LON_ADDR + 2, (longitude >> 8) & 0xFF);
  EEPROM.write(EEPROM_LOCATION_LON_ADDR + 3, longitude & 0xFF);
  
  EEPROM.commit();
}

} // namespace MeshCore
