#include "LoRaReceiver.h"
#include "../core/Logger.h"
#include "../mesh/PacketDispatcher.h"
#include "LoRaTransmitter.h"

static RadioEvents_t radioEvents;

static void onTxDone();
static void onTxTimeout();

LoRaReceiver& LoRaReceiver::getInstance() {
    static LoRaReceiver instance;
    return instance;
}

void LoRaReceiver::initialize() {
    LOG_DEBUG("Setting up radio event callbacks");
    radioEvents.RxDone = onRxDone;
    radioEvents.RxTimeout = onRxTimeout;
    radioEvents.RxError = onRxError;
    radioEvents.TxDone = onTxDone;
    radioEvents.TxTimeout = onTxTimeout;
    
    LOG_DEBUG("Initializing radio hardware");
    Radio.Init(&radioEvents);
    
    LOG_INFO_FMT("Setting LoRa frequency to %lu Hz", Config::LoRa::FREQUENCY);
    Radio.SetChannel(Config::LoRa::FREQUENCY);
    
    LOG_DEBUG("Configuring LoRa radio parameters");
    Radio.SetRxConfig(
        MODEM_LORA,
        Config::LoRa::BANDWIDTH,
        Config::LoRa::SPREADING_FACTOR,
        Config::LoRa::CODING_RATE,
        0,
        Config::LoRa::PREAMBLE_LENGTH,
        0,
        Config::LoRa::FIXED_LENGTH_PAYLOAD,
        0,
        true,
        0,
        0,
        Config::LoRa::IQ_INVERSION,
        true
    );
    
    LOG_DEBUG_FMT("Setting sync word to 0x%02X", Config::LoRa::SYNC_WORD);
    Radio.SetSyncWord(Config::LoRa::SYNC_WORD);
    
    LOG_DEBUG_FMT("Configuring LoRa TX parameters (power: %d dBm)", Config::LoRa::TX_POWER);
    Radio.SetTxConfig(
        MODEM_LORA,
        Config::LoRa::TX_POWER,
        0,
        Config::LoRa::BANDWIDTH,
        Config::LoRa::SPREADING_FACTOR,
        Config::LoRa::CODING_RATE,
        Config::LoRa::PREAMBLE_LENGTH,
        Config::LoRa::FIXED_LENGTH_PAYLOAD,
        true,
        0,
        0,
        Config::LoRa::IQ_INVERSION,
        Config::LoRa::TX_TIMEOUT_MS
    );
    
    LOG_INFO("Starting continuous reception");
    Radio.Rx(0);
}

void LoRaReceiver::processQueue() {
    MeshCore::QueuedPacket queuedPacket;
    
    while (getInstance().packetQueue.dequeue(queuedPacket)) {
        MeshCore::PacketEvent event(queuedPacket.packet, queuedPacket.rssi, 
                                    queuedPacket.snr, queuedPacket.timestamp);
        MeshCore::PacketDispatcher::getInstance().dispatchPacket(event);
    }
}

void LoRaReceiver::onRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr) {
    LOG_DEBUG_FMT("Packet received: %d bytes, RSSI: %d dBm, SNR: %d dB", size, rssi, snr);
    
    MeshCore::DecodedPacket packet;
    bool decoded = MeshCore::PacketDecoder::decode(payload, size, packet);
    
    if (decoded) {
        uint32_t timestamp = millis();
        getInstance().packetQueue.enqueue(packet, rssi, snr, timestamp);
    } else {
        LOG_WARN("Failed to decode packet");
    }
    
    Radio.Rx(0);
}

void LoRaReceiver::onRxTimeout() {
    LOG_DEBUG("RX timeout, restarting reception");
    Radio.Rx(0);
}

void LoRaReceiver::onRxError() {
    LOG_WARN("RX error occurred, restarting reception");
    Radio.Rx(0);
}

static void onTxDone() {
    LoRaTransmitter& tx = LoRaTransmitter::getInstance();
    tx.notifyTxComplete(true);
    LOG_DEBUG("TX complete, returning to RX");
    Radio.Rx(0);
}

static void onTxTimeout() {
    LoRaTransmitter& tx = LoRaTransmitter::getInstance();
    tx.notifyTxComplete(false);
    LOG_WARN("TX timeout, returning to RX");
    Radio.Rx(0);
}

