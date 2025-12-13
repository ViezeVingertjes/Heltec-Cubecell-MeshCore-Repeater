// System/library includes first
#include <Arduino.h>

// Project includes
#include "core/Config.h"
#include "core/CryptoIdentity.h"
#include "core/Logger.h"
#include "core/NodeConfig.h"
#include "mesh/channels/PrivateChannelAnnouncer.h"
#include "mesh/channels/PublicChannelAnnouncer.h"
#include "mesh/processors/Deduplicator.h"
#include "mesh/processors/PacketForwarder.h"
#include "mesh/processors/PacketLogger.h"
#include "mesh/processors/StatusResponder.h"
#include "mesh/processors/AdvertResponder.h"
#include "mesh/processors/TraceHandler.h"
#include "mesh/processors/DiscoveryResponder.h"
#include "power/PowerManager.h"
#include "radio/LoRaReceiver.h"
#include "radio/LoRaTransmitter.h"

static MeshCore::Deduplicator deduplicator;
static MeshCore::PacketLogger packetLogger;
static MeshCore::TraceHandler traceHandler;
static MeshCore::PacketForwarder packetForwarder;
static StatusResponder statusResponder;
static AdvertResponder advertResponder;
static MeshCore::DiscoveryResponder discoveryResponder;

void setup() {
#ifdef ENABLE_LOGGING
  logger.begin();
  logger.setLevel(LogLevel::DEBUG);
#endif

  LOG_INFO("=== CubeCell MeshCore Starting ===");
  LOG_INFO_FMT("Firmware: v%s (built %s)", FIRMWARE_VERSION, FIRMWARE_BUILD_DATE);

  CryptoIdentity::getInstance().initialize();
  PublicChannelAnnouncer::getInstance().initialize();
  PrivateChannelAnnouncer::getInstance().initialize();

  PowerManager::getInstance().initialize();

  MeshCore::NodeConfig::getInstance().initialize();

  MeshCore::PacketDispatcher &dispatcher =
      MeshCore::PacketDispatcher::getInstance();
  dispatcher.addProcessor(&deduplicator);
  dispatcher.addProcessor(&packetLogger);
  dispatcher.addProcessor(&statusResponder);
  dispatcher.addProcessor(&advertResponder);
  dispatcher.addProcessor(&discoveryResponder);

  if (Config::Forwarding::ENABLED) {
    dispatcher.addProcessor(&traceHandler);
    dispatcher.addProcessor(&packetForwarder);

    uint16_t nodeId = MeshCore::NodeConfig::getInstance().getNodeId();
    uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
    LOG_INFO_FMT("Forwarding ENABLED - Node ID: 0x%04X, Hash: 0x%02X", nodeId,
                 nodeHash);
    LOG_INFO_FMT("RX Delay: %.2f, TX Delay: %.2f",
                 Config::Forwarding::RX_DELAY_BASE,
                 Config::Forwarding::TX_DELAY_FACTOR);
  } else {
    LOG_INFO("Forwarding DISABLED");
  }

  LOG_INFO_FMT("Registered %d packet processors",
               dispatcher.getProcessorCount());

  LoRaReceiver &receiver = LoRaReceiver::getInstance();
  receiver.initialize();
  LOG_INFO("LoRa receiver initialized");

  LoRaTransmitter &transmitter = LoRaTransmitter::getInstance();
  transmitter.initialize();
  transmitter.registerTxCallbacks();
  LOG_INFO("LoRa transmitter initialized");

  LOG_INFO("Setup complete");
}

void loop() {
  // Critical path - always execute
  Radio.IrqProcess();
  LoRaReceiver::getInstance().processQueue();

  // Forwarding (if enabled)
  if (Config::Forwarding::ENABLED) {
    packetForwarder.loop();
  }

  // Status responder (only process if has pending response)
  statusResponder.loop();
  
  // Advert responder (only process if has pending response)
  advertResponder.loop();
  
  // Discovery responder (only process if has pending response)
  discoveryResponder.loop();

  // Power management - sleep when possible
  if (Config::Power::LIGHT_SLEEP_ENABLED) {
    bool hasPendingWork = (Config::Forwarding::ENABLED && 
                           packetForwarder.hasPendingPackets()) ||
                          statusResponder.hasPendingResponse() ||
                          advertResponder.hasPendingResponse() ||
                          discoveryResponder.hasPendingResponse();
    if (!hasPendingWork) {
      PowerManager::getInstance().sleep();
    }
  }
}
