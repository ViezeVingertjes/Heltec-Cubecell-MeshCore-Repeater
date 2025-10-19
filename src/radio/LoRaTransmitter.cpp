#include "LoRaTransmitter.h"
#include "../core/Logger.h"
#include <Arduino.h>
#include <math.h>

LoRaTransmitter& LoRaTransmitter::getInstance() {
    static LoRaTransmitter instance;
    return instance;
}

void LoRaTransmitter::initialize() {
    LOG_DEBUG("Initializing LoRa transmitter");
    nextAllowedTxTime = 0;
    LOG_INFO("LoRa transmitter ready");
}

bool LoRaTransmitter::transmit(const uint8_t* data, uint16_t length) {
    if (transmitting) {
        LOG_WARN("Transmit rejected - already transmitting");
        return false;
    }
    
    if (!canTransmitNow()) {
        uint32_t waitTime = nextAllowedTxTime - millis();
        LOG_DEBUG_FMT("In silence period, must wait %lu ms", waitTime);
        return false;
    }
    
    if (data == nullptr || length == 0 || length > 255) {
        LOG_ERROR("Invalid transmit parameters");
        failureCount++;
        return false;
    }
    
    LOG_DEBUG_FMT("Transmitting %d bytes", length);
    
    transmitting = true;
    txStartTime = millis();
    transmitCount++;
    Radio.Send((uint8_t*)data, length);
    
    return true;
}

bool LoRaTransmitter::canTransmitNow() const {
    return millis() >= nextAllowedTxTime;
}

void LoRaTransmitter::notifyTxComplete(bool success) {
    transmitting = false;
    
    if (success) {
        uint32_t airtime = millis() - txStartTime;
        totalAirtimeMs += airtime;
        
        uint32_t silencePeriod = static_cast<uint32_t>(airtime * Config::Forwarding::AIRTIME_BUDGET_FACTOR);
        nextAllowedTxTime = millis() + silencePeriod;
        
        LOG_DEBUG_FMT("TX complete, airtime: %lu ms, silence until: %lu ms", airtime, silencePeriod);
    } else {
        failureCount++;
    }
}

uint32_t LoRaTransmitter::estimateAirtime(uint16_t packetLength) {
    uint32_t bw = 125000;
    switch (Config::LoRa::BANDWIDTH) {
        case 0: bw = 125000; break;
        case 1: bw = 250000; break;
        case 2: bw = 500000; break;
        case 3: bw = 62500; break;
        default: bw = 125000; break;
    }
    
    float sf = Config::LoRa::SPREADING_FACTOR;
    float cr = Config::LoRa::CODING_RATE;
    
    float symbolDuration = (1 << static_cast<int>(sf)) / static_cast<float>(bw) * 1000.0f;
    
    float preambleSymbols = Config::LoRa::PREAMBLE_LENGTH + 4.25f;
    
    float payloadSymbols = 8.0f + max(
        ceil((8.0f * packetLength - 4.0f * sf + 28.0f + 16.0f) / (4.0f * sf)) * (cr + 4.0f), 
        0.0f
    );
    
    float totalSymbols = preambleSymbols + payloadSymbols;
    uint32_t airtime = static_cast<uint32_t>(totalSymbols * symbolDuration);
    
    return airtime;
}

