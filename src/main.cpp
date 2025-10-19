#include <Arduino.h>
#include "core/Config.h"
#include "core/NodeConfig.h"
#include "radio/LoRaReceiver.h"
#include "radio/LoRaTransmitter.h"
#include "core/Logger.h"
#include "mesh/processors/Deduplicator.h"
#include "mesh/processors/PacketLogger.h"
#include "mesh/processors/PacketStats.h"
#include "mesh/processors/TraceHandler.h"
#include "mesh/processors/PacketForwarder.h"

static MeshCore::Deduplicator deduplicator;
static MeshCore::PacketLogger packetLogger;
static MeshCore::PacketStats packetStats;
static MeshCore::TraceHandler traceHandler;
static MeshCore::PacketForwarder packetForwarder;

void setup() {
    logger.begin();
    
    LOG_INFO("=== CubeCell MeshCore Starting ===");
    
    MeshCore::NodeConfig::getInstance().initialize();
    
    MeshCore::PacketDispatcher& dispatcher = MeshCore::PacketDispatcher::getInstance();
    dispatcher.addProcessor(&deduplicator);
    dispatcher.addProcessor(&packetLogger);
    dispatcher.addProcessor(&packetStats);
    dispatcher.addProcessor(&traceHandler);
    dispatcher.addProcessor(&packetForwarder);
    
    LOG_INFO_FMT("Registered %d packet processors", dispatcher.getProcessorCount());
    
    if (Config::Forwarding::ENABLED) {
        uint16_t nodeId = MeshCore::NodeConfig::getInstance().getNodeId();
        uint8_t nodeHash = MeshCore::NodeConfig::getInstance().getNodeHash();
        LOG_INFO_FMT("Forwarding ENABLED - Node ID: 0x%04X, Hash: 0x%02X", nodeId, nodeHash);
        LOG_INFO_FMT("RX Delay: %.2f, TX Delay: %.2f, Airtime Factor: %.2f",
                     Config::Forwarding::RX_DELAY_BASE, 
                     Config::Forwarding::TX_DELAY_FACTOR,
                     Config::Forwarding::AIRTIME_BUDGET_FACTOR);
    } else {
        LOG_INFO("Forwarding DISABLED");
    }
    
    LoRaReceiver& receiver = LoRaReceiver::getInstance();
    receiver.initialize();
    LOG_INFO("LoRa receiver initialized");
    
    LoRaTransmitter& transmitter = LoRaTransmitter::getInstance();
    transmitter.initialize();
    LOG_INFO("LoRa transmitter initialized");
    
    LOG_INFO("Setup complete");
}

void loop() {
    Radio.IrqProcess();
    LoRaReceiver::getInstance().processQueue();
    packetForwarder.loop();
}
