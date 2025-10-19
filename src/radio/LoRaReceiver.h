#pragma once

#include "LoRaWan_APP.h"
#include "../core/Config.h"
#include "../core/PacketDecoder.h"
#include "../mesh/PacketQueue.h"

class LoRaReceiver {
public:
    static LoRaReceiver& getInstance();
    
    void initialize();
    void processQueue();
    
    MeshCore::PacketQueue& getQueue() { return packetQueue; }
    
private:
    LoRaReceiver() = default;
    
    static void onRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr);
    static void onRxTimeout();
    static void onRxError();
    
    MeshCore::PacketQueue packetQueue;
    
    LoRaReceiver(const LoRaReceiver&) = delete;
    LoRaReceiver& operator=(const LoRaReceiver&) = delete;
};

